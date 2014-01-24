/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso.cpp -> implements routines for iso surface
 -------------------------------------------------------------------------*/
#include "iso.hpp"
#include "csg.hpp"
#include "stl.hpp"
#include "qef.hpp"

namespace q {
namespace iso {
mesh::~mesh() {
  if (m_pos) FREE(m_pos);
  if (m_nor) FREE(m_nor);
  if (m_index) FREE(m_index);
}

static const float DEFAULT_GRAD_STEP = 1e-3f;
vec3f gradient(const csg::node &node, const vec3f &pos, float grad_step = DEFAULT_GRAD_STEP) {
  const auto dx = vec3f(grad_step, 0.f, 0.f);
  const auto dy = vec3f(0.f, grad_step, 0.f);
  const auto dz = vec3f(0.f, 0.f, grad_step);
  const aabb box = aabb::all(); //(pos-2.f*grad_step, pos+2.f*grad_step);
  const auto c = csg::dist(pos, node, box);
  const float dndx = csg::dist(pos-dx, node, box);
  const float dndy = csg::dist(pos-dy, node, box);
  const float dndz = csg::dist(pos-dz, node, box);
  const auto n = vec3f(c-dndx, c-dndy, c-dndz);
  if (n==vec3f(zero))
    return vec3f(zero);
  else
    return normalize(n);
}

static const float TOLERANCE_DENSITY = 1e-3f;
static const float TOLERANCE_DIST2   = 1e-8f;
static const int MAX_STEPS = 4;

static vec3f falsepos(const csg::node &node, const grid &grid,
                      const vec3f &org, vec3f p0, vec3f p1,
                      float v0, float v1,float scale)
{
  if (abs(v0) < 1e-3f) return p0;
  if (abs(v1) < 1e-3f) return p1;
  if (v1 < 0.f) {
    swap(p0,p1);
    swap(v0,v1);
  }

  vec3f p;
  loopi(MAX_STEPS) {
    p = p1 - v1 * (p1 - p0) / (v1 - v0);
    const auto density = csg::dist(org+grid.m_cellsize*scale*p, node) + 1e-4f;
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
static const u32 quadtotris_cc[] = {0,2,1,0,3,2};
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
static const u32 octreechildmap[8] = {0, 4, 3, 7, 1, 5, 2, 6};

static const vec3i neighbors[] = {
  // we share a face with these ones
  vec3i(+1,0,0), vec3i(-1,0,0), vec3i(0,+1,0),
  vec3i(0,-1,0), vec3i(0,0,+1), vec3i(0,0,-1),

  // we share one edge only with these ones
  vec3i(+1,+1,0), vec3i(+1,-1,0), vec3i(-1,-1,0), vec3i(+1,-1,0),
  vec3i(+1,0,+1), vec3i(+1,0,-1), vec3i(-1,0,-1), vec3i(+1,0,-1),
  vec3i(0,+1,+1), vec3i(0,+1,-1), vec3i(0,-1,-1), vec3i(0,+1,-1),

  // we share one corner only with these ones
  vec3i(+1,+1,+1), vec3i(+1,+1,-1), vec3i(+1,-1,-1), vec3i(+1,-1,+1),
  vec3i(-1,+1,+1), vec3i(-1,+1,-1), vec3i(-1,-1,-1), vec3i(-1,-1,+1)
};
static const u32 faceneighbornum = 6;
static const u32 edgeneighbornum = 12;
static const u32 lodneighbornum = 18;

static const float SHARP_EDGE = 0.2f;
static const u32 SUBGRID = 8;
static const u32 MAX_NEW_VERT = 8;
static const u32 NOINDEX = ~0x0u;
static const u32 NOTRIANGLE = ~0x0u-1;
static const u32 BORDER = 0x80000000u;
static const u32 OUTSIDE = 0x40000000u;
INLINE u32 isborder(u32 x) { return (x & BORDER) != 0; }
INLINE u32 isoutside(u32 x) { return (x & OUTSIDE) != 0; }
INLINE u32 unpackidx(u32 x) { return x & ~(OUTSIDE|OUTSIDE); }

struct mesh_processor {
  struct meshedge {
    u32 vertex[2];
    u32 face[2];
  };

  void set(vector<vec3f> &p, vector<vec3f> &n, vector<u32> &t,
           vector<pair<u32,u32>> &cracks,
           int first_vert, int first_idx)
  {
    m_pos_buffer = &p;
    m_nor_buffer = &n;
    m_idx_buffer = &t;
    m_cracks = &cracks;
    m_first_vert = first_vert;
    m_first_idx = first_idx;
    m_vertnum = (p.length() - m_first_vert);
    m_trinum = (t.length() - m_first_idx) / 3;
    m_edges.setsize(6*m_trinum);
    m_edgelists.setsize(m_vertnum + 6*m_trinum);
    m_list.setsize(m_vertnum + 6*m_trinum);
  }

  void append(int &edgenum, int i1, int i2, int face0, int face1) {
    auto &e = m_edges[edgenum];
    auto nextedge = &m_edgelists[0] + m_vertnum;
    e.vertex[0] = i1;
    e.vertex[1] = i2;
    e.face[0] = face0;
    e.face[1] = face1;

    u32 idx = m_edgelists[i1];
    if (idx == NOINDEX)
      m_edgelists[i1] = edgenum;
    else for (;;) {
      const u32 index = nextedge[idx];
      if (index == NOINDEX) {
        nextedge[idx] = edgenum;
        break;
      }
      idx = index;
    }
    nextedge[edgenum++] = NOINDEX;
  }

  pair<int,int> buildedges() {
    const auto &t = *m_idx_buffer;
    auto nextedge = &m_edgelists[0] + m_vertnum;

    loopi(m_vertnum) m_edgelists[i] = NOINDEX;

    // step 1 - find all edges with first index smaller than the second one
    int edgenum = 0;
    loopi(m_trinum) {
      const auto tri = &t[m_first_idx + 3*i];
      int i1 = unpackidx(tri[2]) - m_first_vert;
      loopj(3) {
        int i2 = unpackidx(tri[j]) - m_first_vert;
        // if (i1 == 36 || i2 == 36) DEBUGBREAK;
        if (i1 < i2) append(edgenum, i1, i2, i, i);
        i1 = i2;
      }
    }

    // step 2 - go over all edges with first index greater than the second one
    loopi(m_trinum) {
      const auto tri = &t[m_first_idx + 3*i];
      int i1 = unpackidx(tri[2]) - m_first_vert;
      loopj(3) {
        int i2 = unpackidx(tri[j]) - m_first_vert;
        if (i1 > i2) {
          u32 idx = m_edgelists[i2];
          for (; idx != NOINDEX; idx = nextedge[idx]) {
            auto e = &m_edges[idx];
            if (e->vertex[1] == u32(i1) && e->face[0] == e->face[1]) {
              e->face[1] = i;
              break;
            }
          }

          // this is an edge with one triangle only. we append it now
          if (idx == NOINDEX) append(edgenum, i2, i1, i, i);
        }
        i1 = i2;
      }
    }
    const int onedirnum = edgenum;

    // step 3 - append all edges with i2 > i1. Be careful to not push several
    // times the same edge. We stopped as soon as we see an newly pushed edge
    loopi(m_vertnum) {
      u32 idx = m_edgelists[i];
      for (; idx != NOINDEX; idx = nextedge[idx]) {
        auto &e = m_edges[idx];
        if (e.vertex[0] > e.vertex[1]) break;
        if (e.vertex[0] == e.vertex[1]) continue;
        append(edgenum, e.vertex[1], e.vertex[0], e.face[0], e.face[1]);
      }
    }
    return makepair(edgenum, onedirnum);
  }

  INLINE vec3f trinormalu(u32 idx) {
    auto &t = *m_idx_buffer;
    auto &p = *m_pos_buffer;
    const auto t0 = unpackidx(t[3*idx+0]);
    const auto t1 = unpackidx(t[3*idx+1]);
    const auto t2 = unpackidx(t[3*idx+2]);
    return cross(p[t0]-p[t2], p[t0]-p[t1]);
  }
  INLINE vec3f trinormal(u32 idx) { return normalize(trinormalu(idx)); }

  void crease(u32 edgenum) {
    auto &t = *m_idx_buffer;
    auto &p = *m_pos_buffer;
    auto &n = *m_nor_buffer;
    const u32 firsttri = m_first_idx / 3;

    // step 1 - find sharp vertices edges they belong to
    loopi(m_vertnum) m_list[i].first = m_list[i].second = NOINDEX;
    loopi(edgenum) {
      const auto idx0 = m_edges[i].face[0], idx1 = m_edges[i].face[1];
      if (idx0 == idx1) continue;
      const auto n0 = trinormal(firsttri+idx0), n1 = trinormal(firsttri+idx1);
      if (dot(n0, n1) > SHARP_EDGE) continue;
      loopj(2) {
        const auto idx = m_edges[i].vertex[j];
        m_list[idx].second = NOTRIANGLE;
      }
    }

    // step 2 - chain triangles that contain sharp vertices
    u32 listindex = m_vertnum;
    loopi(m_trinum) loopj(3) {
      const auto idx = unpackidx(t[3*(firsttri+i)+j]) - m_first_vert;
      if (m_list[idx].second == NOINDEX) continue;
      m_list[listindex].first = m_list[idx].first;
      m_list[listindex].second = m_list[idx].second;
      m_list[idx].first = listindex++;
      m_list[idx].second = i+firsttri;
    }

    // step 3 - go over all "sharp" vertices and duplicate them as needed
    loopi(m_vertnum) {
      if (m_list[i].first == NOINDEX) continue;

      // compute the number of new vertices we may create at most
      auto curr = m_list[m_list[i].first];
      u32 maxvertnum = 1;
      while (curr.first != NOINDEX) {
        ++maxvertnum;
        curr = m_list[curr.first];
      }
      m_new_verts.setsize(maxvertnum);

      // we already have a valid vertex that we can use
      m_new_verts[0] = makepair(trinormal(m_list[i].second), u32(m_first_vert+i));
      u32 vertnum = 1;

      // go over all triangles and find best vertex for each of them
      curr = m_list[m_list[i].first];
      while (curr.first != NOINDEX) {
        const auto nor = trinormal(curr.second);
        u32 matched = 0;
        for (; matched < vertnum; ++matched)
          if (dot(nor, m_new_verts[matched].first) > SHARP_EDGE)
            break;

        // we need to create a new vertex for this triangle
        if (matched == vertnum) {
          m_new_verts[vertnum++] = makepair(nor, u32(p.length()));
          p.add(p[m_first_vert+i]);
          n.add(vec3f(zero));
        }

        // update the index in the triangle
        loopj(3) {
          const auto idx = t[3*curr.second+j];
          if (unpackidx(idx) == u32(m_first_vert+i)) {
            u32 mask = isoutside(idx) ? OUTSIDE : 0;
            mask |= isborder(idx) ? BORDER : 0;
            t[3*curr.second+j] = mask | m_new_verts[matched].second;
          }
        }
        curr = m_list[curr.first];
      }
    }

    // step 4 - recompute vertex normals (weighted by triangle areas)
    m_vertnum = p.length() - m_first_vert;
    loopi(m_vertnum) n[m_first_vert+i] = vec3f(zero);
    loopi(m_trinum) {
      const auto idx = firsttri+i;
      const auto tri = &t[3*idx];
      const auto nor = trinormalu(idx);
      loopj(3) n[unpackidx(tri[j])] += nor;
    }
    loopi(m_vertnum) n[m_first_vert+i] = abs(normalize(n[m_first_vert+i]));
  }

  INLINE bool containsother(const int *tri, int other) {
    loopk(3) if (tri[k] == other) return true;
    return false;
  }

  u32 findthirdindex(u32 v0, u32 v1, bool &switchv0v1) {
    auto nextedge = &m_edgelists[0] + m_vertnum;
    auto l0 = m_edgelists[v0 - m_first_vert];
    auto &t = *m_idx_buffer;
    for (; l0 != int(NOINDEX); l0 = nextedge[l0]) {
      const auto &e0 = m_edges[l0];
      loopj(2) {
        const auto tri = &t[3*e0.face[j] + m_first_idx];
        bool foundit = false;
        u32 v2 = v0;
        u32 idx[3]={0,0,0};
        loopk(3) {
          const int unpacked = unpackidx(tri[k]);
          if (unpacked == int(v0)) idx[0] = k;
          else if (unpacked == int(v1)) {
            idx[1] = k;
            foundit = true;
          } else {
            idx[2] = k;
            v2 = unpacked;
          }
        }

        // see if we need to reorder v0 and v1 to get proper orientation
        if (foundit) {
          u32 v0pos = 2;
          if (idx[0] == 0) v0pos = 0;
          else if (idx[1] == 0) v0pos = 1;
          switchv0v1 = idx[(v0pos+1)%3] == 1;
          return v2;
        }
      }
    }
    return v0;
  }

  void fillcracks(u32 edgenum) {
    if (m_cracks->length() == 0) return;

    // Try to fix all cracks by creating a new triangle that fills it
    auto &cracks = *m_cracks;
    auto &t = *m_idx_buffer;
    auto nextedge = &m_edgelists[0] + m_vertnum;
    loopv(cracks) {
      bool switchv0v1 = false;
      auto v0 = cracks[i].first;
      auto v1 = cracks[i].second;
      auto v2 = findthirdindex(v0,v1,switchv0v1);
      auto l0 = m_edgelists[v0 - m_first_vert];
      for (; l0 != int(NOINDEX); l0 = nextedge[l0]) {
        // go over all triangles and try to find a vertex that is connected to
        // both crack vertices
        const auto &e0 = m_edges[l0];
        auto candidate = e0.vertex[0] + m_first_vert;
        if (candidate == v0) candidate = e0.vertex[1] + m_first_vert;
        if (candidate == v1 || candidate == v2) continue;

        // visit the complete edge list from v1 to see if it is connected to
        // the candidate
        auto l1 = m_edgelists[v1 - m_first_vert];
        for (; l1 != int(NOINDEX); l1 = nextedge[l1]) {
          const auto &e1 = m_edges[l1];
          const auto ev0 = e1.vertex[0] + m_first_vert;
          const auto ev1 = e1.vertex[1] + m_first_vert;
          if (ev0 == candidate || ev1 == candidate)
            break;
        }

        // if we find it, fill the crack with this new triangle
        if (l1 != int(NOINDEX)) {
          t.add(switchv0v1?v1:v0);
          t.add(switchv0v1?v0:v1);
          t.add(candidate);
          ++m_trinum;
          break;
        }
      }
    }
  }

  vector<meshedge> m_edges;
  vector<int> m_edgelists;
  vector<pair<u32,u32>> m_list;
  vector<pair<vec3f,u32>> m_new_verts;
  vector<vec3f> *m_pos_buffer;
  vector<vec3f> *m_nor_buffer;
  vector<u32> *m_idx_buffer;
  vector<pair<u32,u32>> *m_cracks;
  int m_first_idx, m_first_vert;
  int m_vertnum, m_trinum;
};

struct octree {
  struct node {
    INLINE node() :
      m_children(NULL), m_first_idx(0), m_first_vert(0),
      m_idxnum(0), m_vertnum(0), m_isleaf(0), m_empty(0), m_level(0)
    {}
    ~node() { SAFE_DELA(m_children); }
    node *m_children;
    u32 m_first_idx, m_first_vert;
    u32 m_idxnum:15, m_vertnum:15;
    u32 m_isleaf:1, m_empty:1;
    u32 m_level;
  };

  INLINE octree(u32 dim) : m_dim(dim), m_logdim(ilog2(dim)) {}
  pair<const node*,u32> findleaf(vec3i xyz) const {
    if (any(xyz<vec3i(zero)) || any(xyz>=vec3i(m_dim)))
      return makepair((const struct node*) NULL, 0u);
    int level = 0;
    const node *node = &m_root;
    for (;;) {
      if (node->m_isleaf) return makepair(node, u32(level));
      assert(m_logdim > u32(level));
      const auto logsize = vec3i(m_logdim-level-1);
      const auto bits = xyz >> logsize;
      const auto child = octreechildmap[bits.x | (bits.y<<1) | (bits.z<<2)];
      assert(all(xyz >= icubev[child] << logsize));
      xyz -= icubev[child] << logsize;
      node = node->m_children + child;
      ++level;
    }
    assert("unreachable" && false);
    return makepair((const struct node*)(NULL),0u);
  }

  static const u32 MAX_ITEM_NUM = (1<<15)-1;
  node m_root;
  u32 m_dim, m_logdim;
};

struct dc_gridbuilder {
  dc_gridbuilder(const csg::node &node, const vec3i &dim) :
    m_node(node), m_grid(vec3f(one), vec3f(zero), dim),
    m_field((dim.x+7)*(dim.y+7)*(dim.z+7)),
    m_lod((dim.x+7)*(dim.y+7)*(dim.z+7)),
    m_qef_index((dim.x+6)*(dim.y+6)*(dim.z+6)),
    m_octree(NULL),
    m_iorg(zero),
    m_level(0)
  {}

  struct qef_output {
    INLINE qef_output(vec3f p, vec3f n, bool valid) : p(p),n(n),valid(valid) {}
    vec3f p, n;
    bool valid;
  };

  INLINE void setoctree(const octree &o) { m_octree = &o; }
  INLINE void setorg(const vec3f &org) { m_grid.m_org = org; }
  INLINE void setsize(const vec3f &size) { m_grid.m_cellsize = size; }
  INLINE u32 index(const vec3i &xyz) const {
    const vec3i p = xyz + vec3i(2);
    return p.x + (p.y + p.z * (m_grid.m_dim.y+6)) * (m_grid.m_dim.x+6);
  }
  INLINE u32 field_index(const vec3i &xyz) {
    const vec3i p = xyz + vec3i(2);
    return p.x + (p.y + p.z * (m_grid.m_dim.y+7)) * (m_grid.m_dim.x+7);
  }
  INLINE float &field(const vec3i &xyz) { return m_field[field_index(xyz)]; }
  INLINE u32 &lod(const vec3i &xyz) { return m_lod[field_index(xyz)]; }
  void resetbuffer() {
    m_pos_buffer.setsize(0);
    m_nor_buffer.setsize(0);
    m_border_remap.setsize(0);
  }

  void initfield() {
    const vec3i org(-2), dim = m_grid.m_dim+vec3i(5);
    loopxyz(org, dim) {
      const auto p = m_grid.vertex(xyz);
      const aabb box(p-m_grid.m_cellsize, p+m_grid.m_cellsize);
      field(xyz) = csg::dist(p, m_node, box);
    }
  }

  void initqef() {
    const vec3i org(-2), dim = m_grid.m_dim+vec3i(4);
    loopxyz(org, dim) m_qef_index[index(xyz)] = NOINDEX;
  }

  void initlod() {
    const vec3i org(-2), dim = m_grid.m_dim+vec3i(5);
    loopxyz(org, dim) {
      auto &curr = lod(xyz);
      curr = 0;
      loopi(int(lodneighbornum)) {
        const auto p = m_iorg + xyz + 2*neighbors[i];
        const auto leaf = m_octree->findleaf(p);
        if (leaf.first == NULL) continue;
        if (leaf.second < m_level) {
          curr = 1;
          break;
        }
      }
    }
  }

  void tesselate() {
    const vec3i org(zero), dim(m_grid.m_dim + vec3i(4));
    loopxyz(org, dim) {
      const int start_sign = field(xyz) < 0.f ? 1 : 0;

      // look at the three edges that start on xyz
      loopi(3) {
        // figure out the edge size
        const auto qaxis0 = axis[(i+1)%3];
        const auto qaxis1 = axis[(i+2)%3];
        const vec3i q[] = {xyz, xyz-qaxis0, xyz-qaxis0-qaxis1, xyz-qaxis1};
        u32 edgelod = 1;
        loopj(4) edgelod = min(edgelod, lod(q[j]));

        // process the edge only when we get the properly aligned origin.
        // otherwise, we would generate the same face more than one
        if (edgelod == 1 && any(ne(xyz&vec3i(one), vec3i(zero))))
          continue;

        // is it an actual edge?
        const auto delta = axis[i] << int(edgelod);
        const int end_sign = field(xyz+delta) < 0.f ? 1 : 0;
        if (start_sign == end_sign) continue;

        // we found one edge. we output one quad for it
        const auto axis0 = axis[(i+1)%3] << int(edgelod);
        const auto axis1 = axis[(i+2)%3] << int(edgelod);
        vec3i p[] = {xyz, xyz-axis0, xyz-axis0-axis1, xyz-axis1};
        u32 quad[4];
        u32 vertnum = 0;
        loopj(4) {
          u32 plod = lod(p[j]);
          const auto np = p[j] & vec3i(~plod);
          const auto idx = index(np);

          if (m_qef_index[idx] == NOINDEX) {
            mcell cell;
            loopk(8) cell[k] = field(np + (icubev[k]<<int(plod)));
            const float scale = float(1<<plod);
            const auto vert = qef_vertex(cell, np, scale);

            // we have an edge but with a double change of sign. so the higher
            // level voxel has no QEF at all
            if (!vert.valid) continue;

            // point is still valid. we are good to go
            const auto pos = m_grid.vertex(np) + vert.p*scale*m_grid.m_cellsize;
            const auto nor = normalize(vert.n);
            m_qef_index[idx] = m_pos_buffer.length();
            m_border_remap.add(OUTSIDE);
            m_pos_buffer.add(pos);
            m_nor_buffer.add(abs(nor));
          }
          quad[vertnum++] = m_qef_index[idx];
        }
        const u32 msk = any(eq(xyz,vec3i(zero)))||any(xyz>m_grid.m_dim)?OUTSIDE:0u;
        const auto orientation = start_sign==1 ? quadtotris : quadtotris_cc;
        if (vertnum == 4)
          loopj(6) m_idx_buffer.add(quad[orientation[j]]|msk);
        else if (vertnum == 3)
          loopj(3) m_idx_buffer.add(quad[orientation[j]]|msk);
        else if (vertnum == 2 && !isoutside(msk))
          m_cracks.add(makepair(quad[0], quad[1]));
      }
    }
  }

  qef_output qef_vertex(const mcell &cell, const vec3i &xyz, float scale) {
    int cubeindex = 0, num = 0;
    loopi(8) if (cell[i] < 0.0f) cubeindex |= 1<<i;
    if (edgetable[cubeindex] == 0)
      return qef_output(vec3f(zero), vec3f(zero), false);

    // find the vertices where the surface intersects the cube
    const auto org = m_grid.vertex(xyz);
    vec3f p[12], n[12], mass(zero);
    vec3f nor = zero;
    loopi(12) {
      if ((edgetable[cubeindex] & (1<<i)) == 0)
        continue;
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto v0 = cell[idx0], v1 = cell[idx1];
      const auto p0 = fcubev[idx0], p1 = fcubev[idx1];
      p[num] = falsepos(m_node, m_grid, org, p0, p1, v0, v1, scale);
      n[num] = gradient(m_node, org+p[num]*scale*m_grid.m_cellsize);
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
    return qef_output(mass + qef::evaluate(matrix, vector, num), nor, true);
  }

  void removeborders(u32 first_idx, u32 first_vertex) {
    // find the list of vertex which is actually used. initially, they are all
    // considered outside
    const u32 len = m_idx_buffer.length();
    rangei(first_idx, len) {
      const u32 idx = m_idx_buffer[i];
      if (!isoutside(idx)) m_border_remap[idx] = 0;
    }

    // compact the vertex buffers and create the remap table at the same time
    int last = m_border_remap.length()-1, first = first_vertex;
    for (;;) {
      while (first <= last && isoutside(m_border_remap[last])) --last;
      while (first <= last && !isoutside(m_border_remap[first])) {
        m_border_remap[first] = first;
        ++first;
      }
      if (first > last) break;
      else if (first != last) {
        m_border_remap[last] = first;
        m_pos_buffer[first] = m_pos_buffer[last];
        m_nor_buffer[first] = m_nor_buffer[last];
      }
      ++first;
      --last;
    }
    const u32 vert_buf_len = first;

    // compact the index buffer and remap the indices
    last = m_idx_buffer.length()-1, first = first_idx;
    for (;;) {
      // skip two triangles at once (since they will be both outside anyway)
      while (first <= last && isoutside(m_idx_buffer[last])) last -= 3;
      while (first <= last && !isoutside(m_idx_buffer[first])) {
        assert(m_border_remap[m_idx_buffer[first]] < vert_buf_len);
        m_idx_buffer[first] = m_border_remap[m_idx_buffer[first]];
        ++first;
      }
      if (first > last) break;
      assert(last-first >= 3);
      last -= 2;
      loopi(3) {
        assert(isoutside(m_idx_buffer[first]));
        assert(!isoutside(m_idx_buffer[last+i]));
        const u32 idx = unpackidx(m_idx_buffer[last+i]);
        assert(m_border_remap[idx] < vert_buf_len);
        m_idx_buffer[first++] = m_border_remap[idx];
      }
      --last;
    }
    m_pos_buffer.setsize(vert_buf_len);
    m_nor_buffer.setsize(vert_buf_len);
    m_border_remap.setsize(vert_buf_len);
    m_idx_buffer.setsize(first);
    return;
  }

  void build(octree::node &node) {
    const int first_idx = m_idx_buffer.length();
    const int first_vert = m_pos_buffer.length();
    m_cracks.setsize(0);
    initfield();
    initqef();
    initlod();
    tesselate();

    // stop here if we do not create anything
    if (first_vert == m_pos_buffer.length())
      return;

    // if (m_iorg == vec3i(0,120,112)) DEBUGBREAK;
    m_mp.set(m_pos_buffer, m_nor_buffer, m_idx_buffer, m_cracks, first_vert, first_idx);
    const auto edgenum = m_mp.buildedges();
    m_mp.fillcracks(edgenum.second);
    m_mp.crease(edgenum.first);
    m_border_remap.setsize(m_pos_buffer.length());
    removeborders(first_idx, first_vert);
    assert(node.m_vertnum <= octree::MAX_ITEM_NUM);
    assert(node.m_idxnum <= octree::MAX_ITEM_NUM);
    node.m_first_vert = first_vert;
    node.m_first_idx = first_idx;
    node.m_vertnum = m_pos_buffer.length() - first_vert;
    node.m_idxnum = m_idx_buffer.length() - first_idx;
    node.m_isleaf = 1;
  }

  const csg::node &m_node;
  grid m_grid;
  mesh_processor m_mp;
  vector<float> m_field;
  vector<u32> m_lod;
  vector<vec3f> m_pos_buffer, m_nor_buffer;
  vector<pair<u32,u32>> m_cracks;
  vector<u32> m_idx_buffer;
  vector<u32> m_border_remap;
  vector<u32> m_qef_index;
  const octree *m_octree;
  vec3i m_iorg;
  u32 m_level;
};

struct recursive_builder {
  recursive_builder(const csg::node &node, const vec3f &org, float cellsize, u32 dim) :
    s(node, vec3i(SUBGRID)), m_node(node), m_org(org),
    m_cellsize(cellsize),
    m_half_side_len(float(dim) * m_cellsize * 0.5f),
    m_half_diag_len(sqrt(3.f) * m_half_side_len),
    m_dim(dim)
  {
    assert(ispoweroftwo(dim) && dim % SUBGRID == 0);
    m_maxlevel = ilog2(dim / SUBGRID);
  }
  INLINE vec3f pos(const vec3i &xyz) { return m_org+m_cellsize*vec3f(xyz); }
  INLINE void setoctree(octree &o) {m_octree=&o;}

  void build() {
    build(m_octree->m_root);
    recurse(m_octree->m_root);
  }

  void build(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    const auto scale = 1.f / float(1<<level);
    const auto cellnum = m_dim >> level;
    const auto dist = csg::dist(pos(xyz) + scale*vec3f(m_half_side_len), m_node);
    node.m_level = level;
    if (abs(dist) > m_half_diag_len * scale) {
      node.m_isleaf = node.m_empty = 1;
      return;
    }
   // if (cellnum == SUBGRID || (level == 5 && xyz.x >= 32) || (level == 5 && xyz.y >= 16)) {
//     if (cellnum == SUBGRID || (level == 4 && xyz.y <= 16)) {
    // if (cellnum == SUBGRID || (level == 3 && xyz.y > 16))
     if (cellnum == SUBGRID) {
      printf("level %d\n", level);
//      fflush(stdout);
      node.m_isleaf = 1;
    } else {
      node.m_children = NEWAE(octree::node, 8);
      loopi(8) {
        const auto childxyz = xyz+int(cellnum/2)*icubev[i];
        build(node.m_children[i], childxyz, level+1);
      }
      return;
    }
  }

  void recurse(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    if (node.m_empty)
      return;
    else if (node.m_isleaf) {
      const vec3f sz(float(1<<(m_maxlevel-level)) * m_cellsize);
      s.m_octree = m_octree;
      s.m_iorg = xyz;
      s.m_level = level;
      s.setsize(sz);
      s.setorg(pos(xyz));
      s.build(node);
    } else {
      loopi(8) {
        const auto cellnum = m_dim >> level;
        const auto childxyz = xyz+int(cellnum/2)*icubev[i];
        recurse(node.m_children[i], childxyz, level+1);
      }
    }
  }

  octree *m_octree;
  dc_gridbuilder s;
  const csg::node &m_node;
  vec3f m_org;
  float m_cellsize, m_half_side_len, m_half_diag_len;
  u32 m_dim, m_maxlevel;
};

mesh dc_mesh(const vec3f &org, u32 cellnum, float cellsize, const csg::node &d) {
  octree o(cellnum);
  recursive_builder r(d, org, cellsize, cellnum);
  r.setoctree(o);
  r.build();
  const auto p = r.s.m_pos_buffer.move();
  const auto n = r.s.m_nor_buffer.move();
  const auto idx = r.s.m_idx_buffer.move();
  return mesh(p.first, n.first, idx.first, p.second, idx.second);
}
} /* namespace iso */
} /* namespace q */

