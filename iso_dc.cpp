/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - iso_dc.cpp -> implements dual contouring algorithm
-------------------------------------------------------------------------*/
#include "iso.hpp"
#include "stl.hpp"
#include "qef.hpp"

namespace q {
namespace iso {
namespace dc {

static const float TOLERANCE_DENSITY = 1e-3f;
static const float TOLERANCE_DIST2   = 1e-2f;
static const int MAX_STEPS = 8;

static vec3f false_position(distance_field df, const grid &grid, vec3f org, vec3f p0, vec3f p1, float v0, float v1) {
  vec3f p;
  if (v1 < 0.f) {
    swap(p0,p1);
    swap(v0,v1);
  }

  loopi(MAX_STEPS) {
    p = p1 - v1 * (p1 - p0) / (v1 - v0);
    const auto density = df(org+grid.m_cellsize*p) + 1e-4f;
    if (abs(density) < TOLERANCE_DENSITY) break;
    if (distance2(p0,p1) < TOLERANCE_DIST2) break;
    if (density < 0.f) {
      p0 = p;
      v0 = density;
    } else {
      p1 = p;
      v1 = density;
    }
  }
  return p;
}

typedef float mcell[8];
static const vec3i axis[] = {vec3i(1,0,0), vec3i(0,1,0), vec3i(0,0,1)};
static const vec3i faxis[] = {vec3f(1.f,0.f,0.f), vec3i(0.f,1.f,0.f), vec3i(0.f,0.f,1.f)};
static const u32 quadtotris[] = {0,1,2,0,2,3};

struct slicebuilder {
  enum { NOINDEX = ~0x0u };
  slicebuilder(const grid &grid, distance_field df) :
    m_df(df), m_grid(grid),
    m_cube_per_slice((grid.m_dim.x+1)*(grid.m_dim.y+1)),
    m_vertex_per_slice((grid.m_dim.x+2)*(grid.m_dim.y+2)),
    m_field(3*m_vertex_per_slice),
    m_edge(9*m_vertex_per_slice),
    m_qef_pos(2*m_cube_per_slice),
    m_qef_nor(2*m_cube_per_slice),
    m_qef_index(2*m_cube_per_slice)
  {}

  INLINE u32 index(const vec3i &xyz) {
    const auto offset = (xyz.z%2) * m_cube_per_slice;
    return offset+xyz.y*m_grid.m_dim.x+xyz.x;
  }

  INLINE float &field(const vec3i &xyz) {
    const auto &dim = m_grid.m_dim;
    const auto offset = (xyz.z%2)*(dim.x+2)*(dim.y+2);
    return m_field[offset+xyz.y*(dim.x+2)+xyz.x];
  }

  INLINE pair<vec3f,vec3f> &edge(const vec3i &xyz, u32 e) {
    const auto &dim = m_grid.m_dim;
    const auto offset = (e+3*(xyz.z%2))*(dim.x+2)*(dim.y+2);
    return m_edge[offset+xyz.y*(dim.x+2)+xyz.x];
  }

  void init_field(u32 z) {
    const vec2i org(zero), end(m_grid.m_dim.x+2,m_grid.m_dim.y+2);
    loopxy(org, end, z)
      field(xyz) = m_df(m_grid.vertex(xyz));
  }

  void init_edge(u32 z) {
    const vec2i org(zero), end(m_grid.m_dim.x+2,m_grid.m_dim.y+2);
    loopxy(org, end, z) {
      const auto v0 = field(xyz);
      const auto org = m_grid.vertex(xyz);
      const vec3f p0(zero);
      loopi(3) {
        const vec3f p1 = faxis[i];
        const auto v1 = field(xyz+axis[i]);
        const auto p = false_position(m_df, m_grid, org, p0, p1, v0, v1);
        const auto n = gradient(m_df, p);
        edge(xyz,i) = makepair(p,n);
      }
    }
  }

  void init_slice(u32 z) {
    const u32 offset = (z%2)*m_cube_per_slice;
    loopi(m_cube_per_slice) m_qef_index[offset+i] = NOINDEX;
    loopxy(vec2i(zero), vec2i(m_grid.m_dim.x+1,m_grid.m_dim.y+1), z) {
      vec3f pos, nor;
      mcell cell;
      loopi(8) cell[i] = m_df(m_grid.vertex(icubev[i]+xyz));
      if (qef_vertex(cell, xyz, pos, nor)) {
        m_qef_pos[index(xyz)] = m_grid.vertex(xyz)+pos*m_grid.m_cellsize;
        m_qef_nor[index(xyz)] = normalize(nor);
      }
    }
  }

  void tesselate_slice(u32 z) {
    loopxy(vec2i(one), vec2i(m_grid.m_dim.x+1,m_grid.m_dim.y+1), z) {
      const int start_sign = m_df(m_grid.vertex(xyz)) < 0.f ? 1 : 0;
      // look the three edges that start on xyz
      loopi(3) {
        const int end_sign = m_df(m_grid.vertex(xyz+axis[i])) < 0.f ? 1 : 0;
        if (start_sign == end_sign) continue;

        // we found one edge. we output one quad for it
        const auto axis0 = axis[(i+1)%3], axis1 = axis[(i+2)%3];
        const vec3i p[] = {xyz, xyz-axis0, xyz-axis0-axis1, xyz-axis1};
        u32 quad[4];
        loopj(4) {
          const auto idx = index(p[j]);
          if (m_qef_index[idx] == NOINDEX) {
            m_qef_index[idx] = m_positionbuffer.length();
            m_positionbuffer.add(m_qef_pos[idx]);
            m_normalbuffer.add(m_qef_nor[idx]);
          }
          quad[j] = m_qef_index[idx];
        }
        loopj(6) m_indexbuffer.add(quad[quadtotris[j]]);
      }
    }
  }

  bool qef_vertex(const mcell &cell, const vec3i &xyz, vec3f &pos, vec3f &nor) {
    int cubeindex = 0, num = 0;
    loopi(8) if (cell[i] < 0.0f) cubeindex |= 1<<i;

    // cube is entirely in/out of the surface
    if (edgetable[cubeindex] == 0) return false;

    // find the vertices where the surface intersects the cube
    const auto org = m_grid.vertex(xyz);
    vec3f p[12], n[12], mass(zero);
    nor = zero;
    loopi(12) {
      if ((edgetable[cubeindex] & (1<<i)) == 0)
        continue;
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto v0 = cell[idx0], v1 = cell[idx1];
      const auto p0 = fcubev[idx0], p1 = fcubev[idx1];
      p[num] = false_position(m_df, m_grid, org, p0, p1, v0, v1);
      n[num] = gradient(m_df, org+p[num]*m_grid.m_cellsize);
      nor += n[num];
      mass += p[num++];
    }
    mass /= float(num);

    // compute the QEF minimizing point
    double matrix[12][3];
    double vector[12];
    loopi(num) {
      loopj(3) matrix[i][j] = double(n[i][j]);
      const auto d = p[i] - mass;
      vector[i] = double(reduceadd(n[i] * d));
    }
    pos = mass + qef::evaluate(matrix, vector, num);
    nor = abs(normalize(nor));
    return true;
  }

  void build() {
    init_slice(0);
    rangei(1,m_grid.m_dim.z+1) {
      init_slice(i);
      tesselate_slice(i);
    }
  }

  distance_field m_df;
  grid m_grid;
  u32 m_cube_per_slice, m_vertex_per_slice;
  vector<float> m_field;
  vector<pair<vec3f,vec3f>> m_edge;
  vector<vec3f> m_qef_pos, m_qef_nor;
  vector<u32> m_qef_index;
  vector<vec3f> m_positionbuffer;
  vector<vec3f> m_normalbuffer;
  vector<u32> m_indexbuffer;
};
} /* namespace dc */

// fast per-slice dual contouring tesselation
mesh dc_mesh(const grid &grid, distance_field d) {
  dc::slicebuilder s(grid, d);
  s.build();
  const auto p = s.m_positionbuffer.move();
  const auto n = s.m_normalbuffer.move();
  const auto idx = s.m_indexbuffer.move();
  return mesh(p.first, n.first, idx.first, p.second, idx.second);
}
} /* namespace iso */
} /* namespace q */

