/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - iso.hpp -> implements common functions for iso surface tesselation, collision
 - ...etc
 -------------------------------------------------------------------------*/
#include "iso.hpp"
#include "stl.hpp"
#include "qef.hpp"

namespace q {
namespace iso {
mesh::~mesh() {
  if (m_pos) FREE(m_pos);
  if (m_nor) FREE(m_nor);
  if (m_index) FREE(m_index);
}
vec3f gradient(distance_field d, const vec3f &pos, float grad_step) {
  const vec3f dx = vec3f(grad_step, 0.f, 0.f);
  const vec3f dy = vec3f(0.f, grad_step, 0.f);
  const vec3f dz = vec3f(0.f, 0.f, grad_step);
  const float c = d(pos);
  return normalize(vec3f(c-d(pos-dx), c-d(pos-dy), c-d(pos-dz)));
}

static const float TOLERANCE_DENSITY = 1e-3f;
static const float TOLERANCE_DIST2   = 1e-8f;
static const int MAX_STEPS = 4;

static vec3f false_position(distance_field df, const grid &grid, vec3f org, vec3f p0, vec3f p1, float v0, float v1) {
  if (v1 < 0.f) {
    swap(p0,p1);
    swap(v0,v1);
  }

  vec3f p;
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

INLINE pair<vec3i,u32> edge(vec3i start, vec3i end) {
  const auto lower = select(start<end, start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, u32(delta.y+2*delta.z));
}

typedef float mcell[8];
static const vec3i axis[] = {vec3i(1,0,0), vec3i(0,1,0), vec3i(0,0,1)};
static const vec3i faxis[] = {vec3f(1.f,0.f,0.f), vec3i(0.f,1.f,0.f), vec3i(0.f,0.f,1.f)};
static const u32 quadtotris[] = {0,1,2,0,2,3};
static const u16 edgetable[256]= {
  0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
  0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
  0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
  0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
  0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
  0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
  0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
  0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
  0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
  0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
  0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
  0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
  0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
  0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
  0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
  0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
  0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
  0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
  0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
  0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
  0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
  0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
  0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
  0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
  0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
  0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
  0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
  0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
  0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
  0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
  0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
  0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};
static const vec3f fcubev[8] = {
  vec3f(0.f,0.f,0.f)/*0*/, vec3f(0.f,0.f,1.f)/*1*/, vec3f(0.f,1.f,1.f)/*2*/, vec3f(0.f,1.f,0.f)/*3*/,
  vec3f(1.f,0.f,0.f)/*4*/, vec3f(1.f,0.f,1.f)/*5*/, vec3f(1.f,1.f,1.f)/*6*/, vec3f(1.f,1.f,0.f)/*7*/
};
static const vec3i icubev[8] = {
  vec3i(0,0,0)/*0*/, vec3i(0,0,1)/*1*/, vec3i(0,1,1)/*2*/, vec3i(0,1,0)/*3*/,
  vec3i(1,0,0)/*4*/, vec3i(1,0,1)/*5*/, vec3i(1,1,1)/*6*/, vec3i(1,1,0)/*7*/
};
static const int interptable[12][2] = {
  {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
};
static const u32 NOINDEX = ~0x0u;

struct dc_gridbuilder {
  dc_gridbuilder(distance_field df, const vec3i &dim) :
    m_df(df), m_grid(vec3f(one), vec3f(zero), dim),
    m_field(3*(dim.x+2)*(dim.y+2)),
    m_cube_per_slice((dim.x+1)*(dim.y+1)),
    m_qef_pos(2*m_cube_per_slice),
    m_qef_nor(2*m_cube_per_slice),
    m_qef_index(2*m_cube_per_slice)
  {}

  INLINE void setorg(const vec3f &org) { m_grid.m_org = org; }
  INLINE void setsize(const vec3f &size) { m_grid.m_cellsize = size; }
  INLINE u32 index(const vec3i &xyz) const {
    const auto offset = (xyz.z%2) * m_cube_per_slice;
    return offset+xyz.y*(m_grid.m_dim.x+1)+xyz.x;
  }
  INLINE float &field(const vec3i &xyz) {
    const vec2i dim(2+m_grid.m_dim.x, 2+m_grid.m_dim.y);
    const auto offset = (xyz.z%3)*dim.x*dim.y;
    return m_field[offset+dim.x*xyz.y+xyz.x];
  }
  void resetbuffer() {
    m_pos_buffer.setsize(0);
    m_nor_buffer.setsize(0);
  }
  void initfield(u32 z) {
    const vec2i org(zero), dim(2+m_grid.m_dim.x, 2+m_grid.m_dim.y);
    loopxy(org, dim, z) field(xyz) = m_df(m_grid.vertex(xyz));
  }
  void initslice(u32 z) {
    const u32 offset = (z%2)*m_cube_per_slice;
    loopi(m_cube_per_slice) m_qef_index[offset+i] = NOINDEX;
    loopxy(vec2i(zero), vec2i(m_grid.m_dim.x+1,m_grid.m_dim.y+1), z) {
      vec3f pos, nor;
      mcell cell;
      loopi(8) cell[i] = field(icubev[i]+xyz);
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
            m_qef_index[idx] = m_pos_buffer.length();
            m_pos_buffer.add(m_qef_pos[idx]);
            m_nor_buffer.add(m_qef_nor[idx]);
          }
          quad[j] = m_qef_index[idx];
        }
        loopj(6) m_idx_buffer.add(quad[quadtotris[j]]);
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
      vector[i] = double(dot(n[i],d));
    }
    pos = mass + qef::evaluate(matrix, vector, num);
    nor = abs(normalize(nor));
    return true;
  }
  void build() {
    loopi(3) initfield(i);
    initslice(0);
    rangei(1,m_grid.m_dim.z+1) {
      initfield(i+2);
      initslice(i);
      tesselate_slice(i);
    }
  }

  distance_field m_df;
  grid m_grid;
  vector<float> m_field;
  vector<vec3f> m_pos_buffer, m_nor_buffer;
  vector<u32> m_idx_buffer;
  u32 m_cube_per_slice;
  vector<vec3f> m_qef_pos, m_qef_nor;
  vector<u32> m_qef_index;
};

static const u32 SUBGRID = 16;
struct recursive_builder {
  recursive_builder(distance_field df, const vec3f &org, float cellsize, u32 dim) :
    s(df,vec3i(SUBGRID)), m_df(df), m_org(org), m_cellsize(cellsize), m_dim(dim)
  {
    s.setsize(vec3f(cellsize));
    assert(ispoweroftwo(dim) && dim % SUBGRID == 0);
  }
  INLINE vec3f pos(const vec3i &xyz) { return m_org+m_cellsize*vec3f(xyz); }

  void recurse(const vec3i &xyz, u32 cellnum, float half_diag, float half_side_len) {
    const float dist = m_df(pos(xyz) + vec3f(half_side_len));
    if (dist > half_diag) return;
    if (cellnum > SUBGRID) {
      loopi(8) {
        const vec3i childxyz = xyz+int(cellnum/2)*icubev[i];
        recurse(childxyz, cellnum/2, half_diag/2.f, half_side_len/2.f);
      }
      return;
    }
    s.setorg(pos(xyz));
    s.build();
  }

  void build() {
    const auto half_side_len = float(m_dim) * m_cellsize * 0.5f;
    const auto half_diag = sqrt(3.f) * half_side_len;
    recurse(vec3i(zero), m_dim, half_diag, half_side_len);
  }
  dc_gridbuilder s;
  distance_field m_df;
  vec3f m_org;
  float m_cellsize;
  u32 m_dim;
};

mesh dc_mesh(const vec3f &org, u32 cellnum, float cellsize, distance_field d) {
  recursive_builder r(d, org, cellsize, cellnum);
  r.build();
  const auto p = r.s.m_pos_buffer.move();
  const auto n = r.s.m_nor_buffer.move();
  const auto idx = r.s.m_idx_buffer.move();
  return mesh(p.first, n.first, idx.first, p.second, idx.second);
}
} /* namespace iso */
} /* namespace q */

