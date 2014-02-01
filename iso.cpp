/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso.cpp -> implements routines for iso surface
 -------------------------------------------------------------------------*/
#include "iso.hpp"
#include "csg.hpp"
#include "stl.hpp"
#include "qef.hpp"
#include "task.hpp"

STATS(iso_num);
STATS(iso_falsepos_num);
STATS(iso_edge_num);
STATS(iso_gradient_num);
STATS(iso_grid_num);
STATS(iso_octree_num);
STATS(iso_qef_num);
STATS(iso_falsepos);

#define USE_NODE_SIZE 0

#if !defined(NDEBUG)
static void stats() {
  STATS_OUT(iso_num);
  STATS_OUT(iso_qef_num);
  STATS_OUT(iso_edge_num);
  STATS_RATIO(iso_falsepos_num, iso_edge_num);
  STATS_RATIO(iso_falsepos_num, iso_num);
  STATS_RATIO(iso_gradient_num, iso_num);
  STATS_RATIO(iso_grid_num, iso_num);
  STATS_RATIO(iso_octree_num, iso_num);
}
#endif

namespace q {
namespace iso {
mesh::~mesh() {
  if (m_pos) FREE(m_pos);
  if (m_nor) FREE(m_nor);
  if (m_index) FREE(m_index);
}

static const auto DEFAULT_GRAD_STEP = 1e-3f;
static const auto TOLERANCE_DENSITY = 1e-3f;
static const auto TOLERANCE_DIST2 = 1e-8f;
static const int MAX_STEPS = 4;

INLINE pair<vec3i,u32> edge(vec3i start, vec3i end) {
  const auto lower = select(lt(start,end), start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, u32(delta.y+2*delta.z));
}

typedef float mcell[8];
static const vec3i axis[] = {vec3i(1,0,0), vec3i(0,1,0), vec3i(0,0,1)};
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

static const float SHARP_EDGE = 0.2f;
static const u32 SUBGRID = 16;
static const u32 FIELDDIM = SUBGRID+8;
static const u32 QEFDIM = SUBGRID+6;
static const u32 FIELDNUM = FIELDDIM*FIELDDIM*FIELDDIM;
static const u32 QEFNUM = QEFDIM*QEFDIM*QEFDIM;
static const u32 MAX_NEW_VERT = 8;
static const u32 NOINDEX = ~0x0u;
static const u32 NOTRIANGLE = ~0x0u-1;
static const u32 BORDER = 0x80000000u;
static const u32 OUTSIDE = 0x40000000u;
INLINE u32 isborder(u32 x) { return (x & BORDER) != 0; }
INLINE u32 isoutside(u32 x) { return (x & OUTSIDE) != 0; }
INLINE u32 unpackidx(u32 x) { return x & ~(OUTSIDE|BORDER); }
INLINE u32 unpackmsk(u32 x) { return x & (OUTSIDE|BORDER); }

struct edgeitem {
  vec3f org, corg, p0, p1;
  float scale, v0, v1;
};

struct edgestack {
  array<edgeitem,64> it;
  array<vec3f,64> p, pos;
  array<float,64> d;
};

/*-------------------------------------------------------------------------
 - edge creaser and mesh decimation happen here
 -------------------------------------------------------------------------*/
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

  int buildedges() {
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
    return edgenum;
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
          if (unpackidx(idx) == u32(m_first_vert+i))
            t[3*curr.second+j] = unpackmsk(idx) | m_new_verts[matched].second;
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
    //loopi(m_vertnum) n[m_first_vert+i] = vec3f(1.f,0.f,0.f);
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

/*-------------------------------------------------------------------------
 - spatial segmentation used for iso surface extraction
 -------------------------------------------------------------------------*/
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
    if (any(lt(xyz,vec3i(zero))) || any(ge(xyz,vec3i(m_dim))))
      return makepair((const struct node*) NULL, 0u);
    int level = 0;
    const node *node = &m_root;
    for (;;) {
      if (node->m_isleaf) return makepair(node, u32(level));
      assert(m_logdim > u32(level));
      const auto logsize = vec3i(m_logdim-level-1);
      const auto bits = xyz >> logsize;
      const auto child = octreechildmap[bits.x | (bits.y<<1) | (bits.z<<2)];
      assert(all(ge(xyz, icubev[child] << logsize)));
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

INLINE pair<vec3i,int> getedge(const vec3i &start, const vec3i &end, int lod) {
  const auto lower = select(le(start,end), start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, (lod?3:0)+delta.y+2*delta.z);
}

/*-------------------------------------------------------------------------
 - iso surface extraction is done here
 -------------------------------------------------------------------------*/
struct dc_gridbuilder {
  dc_gridbuilder() :
    m_node(NULL),
    m_field(FIELDNUM),
    m_lod(FIELDNUM),
    m_qef_index(QEFNUM),
    m_edge_index(6*FIELDNUM),
    m_stack(NEWE(edgestack)),
    m_octree(NULL),
    m_iorg(zero),
    m_maxlevel(0),
    m_level(0)
  {}
  ~dc_gridbuilder() { DEL(m_stack); }

  struct qef_output {
    INLINE qef_output(vec3f p, vec3f n, bool valid):p(p),n(n),valid(valid){}
    vec3f p, n;
    bool valid;
  };

  INLINE vec3f falsepos(const csg::node &node, vec3f org, vec3f p0, vec3f p1,
                        float v0, float v1, float scale)
  {
    STATS_INC(iso_edge_num);
    if (abs(v0) < TOLERANCE_DENSITY) return p0;
    if (abs(v1) < TOLERANCE_DENSITY) return p1;
    if (v1 < 0.f) {
      swap(p0,p1);
      swap(v0,v1);
    }

    vec3f p;
    loopi(MAX_STEPS) {
      p = p1 - v1 * (p1 - p0) / (v1 - v0);
      const auto pt = csg::dist(node, org+m_cellsize*scale*p);
      const auto density = pt.dist+1e-4f;
      STATS_INC(iso_num);
      STATS_INC(iso_falsepos_num);
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

  INLINE vec3f vertex(const vec3i &p) {
    const vec3i ipos = m_iorg+(p<<(int(m_maxlevel-m_level)));
    return vec3f(ipos)*m_mincellsize;
  }
  INLINE void setoctree(const octree &o) { m_octree = &o; }
  INLINE void setorg(const vec3f &org) { m_org = org; }
  INLINE void setcellsize(float size) { m_cellsize = size; }
  INLINE void setmincellsize(float size) { m_mincellsize = size; }
  INLINE void setnode(const csg::node *node) { m_node = node; }
  INLINE u32 qef_index(const vec3i &xyz) const {
    const vec3i p = xyz + vec3i(2);
    return p.x + (p.y + p.z * QEFDIM) * QEFDIM;
  }
  INLINE u32 field_index(const vec3i &xyz) {
    const vec3i p = xyz + vec3i(2);
    return p.x + (p.y + p.z * FIELDDIM) * FIELDDIM;
  }
  INLINE u32 edge_index(const vec3i &start, int edge) {
    const auto p = start + 2;
    const auto offset = p.x + (p.y + p.z * FIELDDIM) * FIELDDIM;
    return offset + edge * FIELDNUM;
  }

  INLINE float &field(const vec3i &xyz) { return m_field[field_index(xyz)]; }
  void resetbuffer() {
    m_pos_buffer.setsize(0);
    m_nor_buffer.setsize(0);
    m_border_remap.setsize(0);
  }

  void initfield() {
    const vec3i org(-2), dim(FIELDDIM-2);
    stepxyz(org, dim, vec3i(4)) {
      vec3f pos[64];
      float distance[64];
      const auto p = vertex(sxyz);
      const aabb box = aabb(p-2.f*m_cellsize, p+6.f*m_cellsize);
      int index = 0;
      loopxyz(sxyz, sxyz+4) pos[index++] = vertex(xyz);
      assert(index == 64);
      csg::dist(*m_node, pos, distance, 64, box);
      index = 0;
      loopxyz(sxyz, sxyz+4) field(xyz) = distance[index++];
      STATS_ADD(iso_num, 64);
      STATS_ADD(iso_grid_num, 64);
    }
  }

  NOINLINE void initedge() {
    const vec3i org(-2), dim(FIELDDIM-2);
    m_edge_index.memset(0xff);
    m_edges.setsize(0);
    m_delayed_edges.setsize(0);
  }

  NOINLINE void initqef() {
    const vec3i org(-2), dim(QEFDIM-2);
    m_qef_index.memset(0xff);
    m_delayed_qef.setsize(0);
  }

  INLINE u8 &lod(const vec3i &xyz) { return m_lod[field_index(xyz)]; }
  NOINLINE void initlod() {
    m_lod.memset(0);
    const int plod = m_maxlevel - m_level;

#define LOD(NEIGHBOR, ORG, END) do {\
  const vec3i neighbor(m_iorg + NEIGHBOR);\
  const auto leaf = m_octree->findleaf(neighbor);\
  if (leaf.first != NULL && leaf.second < m_level)\
    loopxyz(ORG, END) lod(xyz) = 1;\
} while (0)
    const int p = SUBGRID << plod;
    const int m = -(1<<plod);
    const int ps = SUBGRID-2, ms = -2;
    const int pe = FIELDDIM-2, me = 2;

    // we share a face with these neighbors
    LOD(vec3i(p,0,0), vec3i(ps,ms,ms), vec3i(pe));
    LOD(vec3i(0,p,0), vec3i(ms,ps,ms), vec3i(pe));
    LOD(vec3i(0,0,p), vec3i(ms,ms,ps), vec3i(pe));
    LOD(vec3i(m,0,0), vec3i(ms), vec3i(me,pe,pe));
    LOD(vec3i(0,m,0), vec3i(ms), vec3i(pe,me,pe));
    LOD(vec3i(0,0,m), vec3i(ms), vec3i(pe,pe,me));

    // we share an edge with these neighbors
    LOD(vec3i(p,p,0), vec3i(ps,ps,ms), vec3i(pe));
    LOD(vec3i(p,0,p), vec3i(ps,ms,ps), vec3i(pe));
    LOD(vec3i(0,p,p), vec3i(ms,ps,ps), vec3i(pe));
    LOD(vec3i(m,m,0), vec3i(ms), vec3i(me,me,pe));
    LOD(vec3i(m,0,m), vec3i(ms), vec3i(me,pe,me));
    LOD(vec3i(0,m,m), vec3i(ms), vec3i(pe,me,me));
    LOD(vec3i(p,m,0), vec3i(ps,ms,ms), vec3i(pe,me,pe));
    LOD(vec3i(m,p,0), vec3i(ms,ps,ms), vec3i(me,pe,pe));
    LOD(vec3i(p,0,m), vec3i(ps,ms,ms), vec3i(pe,pe,me));
    LOD(vec3i(m,0,p), vec3i(ms,ms,ps), vec3i(me,pe,pe));
    LOD(vec3i(0,m,p), vec3i(ms,ms,ps), vec3i(pe,me,pe));
    LOD(vec3i(0,p,m), vec3i(ms,ps,ms), vec3i(pe,pe,me));
#undef LOD
  }

  int delayed_qef_vertex(const mcell &cell, const vec3i &xyz, int plod) {
    int cubeindex = 0;
    loopi(8) if (cell[i] < 0.0f) cubeindex |= 1<<i;
    if (edgetable[cubeindex] == 0)
      return 0;
    loopi(12) {
      if ((edgetable[cubeindex] & (1<<i)) == 0)
        continue;
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto e = getedge(icubev[idx0], icubev[idx1], plod);
      auto &idx = m_edge_index[edge_index(xyz+e.first, e.second)];
      if (idx == NOINDEX) {
        idx = m_delayed_edges.length();
        m_delayed_edges.add(makepair(xyz, vec3i(idx0, idx1, plod)));
      }
    }
    return edgetable[cubeindex];
  }

  INLINE void falsepos(const csg::node &node, edgestack &stack, int num) {
    assert(num <= 64);
    auto &it = stack.it;
    auto &pos = stack.pos, &p = stack.p;
    auto &d = stack.d;

    loopk(MAX_STEPS) {
      auto box = aabb::empty();
      loopi(num) {
        p[i] = it[i].p1 - it[i].v1 * (it[i].p1-it[i].p0)/(it[i].v1-it[i].v0);
        pos[i] = it[i].org+m_cellsize*it[i].scale*p[i];
        box.pmin = min(pos[i], box.pmin);
        box.pmax = max(pos[i], box.pmax);
      }
      box.pmin -= 3.f * m_cellsize;
      box.pmax += 3.f * m_cellsize;
      csg::dist(*m_node, &pos[0], &d[0], num, box);
      if (k != MAX_STEPS-1) {
        loopi(num) {
          if (d[i] < 0.f) {
            it[i].p0 = p[i];
            it[i].v0 = d[i];
          } else {
            it[i].p1 = p[i];
            it[i].v1 = d[i];
          }
        }
      } else
        loopi(num) it[i].p0 = p[i];
    }
    STATS_ADD(iso_falsepos_num, num*MAX_STEPS);
    STATS_ADD(iso_num, num*MAX_STEPS);
  }

  void finishedges() {
    const auto len = m_delayed_edges.length();
    STATS_ADD(iso_edge_num, len);

    for (int i = 0; i < len; i += 64) {
      auto &it = m_stack->it;

      // step 1 - run false position with packets of (up-to) 64 points
      const int num = min(64, len-i);
      loopj(num) {
        const auto &e = m_delayed_edges[i+j];
        const auto idx0 = e.second.x, idx1 = e.second.y;
        const auto plod = e.second.z;
        const auto xyz = e.first;
        it[j].org = vertex(xyz);
        it[j].scale = float(1<<plod);
        it[j].p0 = fcubev[idx0];
        it[j].p1 = fcubev[idx1];
        it[j].v0 = field(xyz + (icubev[idx0]<<plod));
        it[j].v1 = field(xyz + (icubev[idx1]<<plod));
        it[j].corg = vec3f(getedge(icubev[idx0], icubev[idx1], plod).first);
        if (it[j].v1 < 0.f) {
          swap(it[j].p0,it[j].p1);
          swap(it[j].v0,it[j].v1);
        }
      }
      falsepos(*m_node, *m_stack, num);

      // step 2 - compute normals for each point using packets of 16x4 points
      const auto dx = vec3f(DEFAULT_GRAD_STEP, 0.f, 0.f);
      const auto dy = vec3f(0.f, DEFAULT_GRAD_STEP, 0.f);
      const auto dz = vec3f(0.f, 0.f, DEFAULT_GRAD_STEP);
      for (int j = 0; j < num; j += 16) {
        const int subnum = min(num-j, 16);
        auto p = &m_stack->p[0];
        auto d = &m_stack->d[0];
        auto box = aabb::empty();
        loopk(subnum) {
          p[4*k]   = it[j+k].org + it[j+k].p0 * it[j+k].scale * m_cellsize;
          p[4*k+1] = p[4*k]-dx;
          p[4*k+2] = p[4*k]-dy;
          p[4*k+3] = p[4*k]-dz;
          box.pmin = min(p[4*k], box.pmin);
          box.pmax = max(p[4*k], box.pmax);
        }
        box.pmin -= 3.f * m_cellsize;
        box.pmax += 3.f * m_cellsize;

        csg::dist(*m_node, p, d, 4*subnum, box);
        STATS_ADD(iso_num, 4*subnum);
        STATS_ADD(iso_gradient_num, 4*subnum);

        loopk(subnum) {
          const auto c    = d[4*k+0];
          const auto dfdx = d[4*k+1];
          const auto dfdy = d[4*k+2];
          const auto dfdz = d[4*k+3];
          const auto grad = vec3f(c-dfdx, c-dfdy, c-dfdz);
          const auto n = grad==vec3f(zero) ? vec3f(zero) : normalize(grad);
          const auto p = it[j+k].p0 - it[j+k].corg;
          m_edges.add(makepair(p,n));
        }
      }
    }
  }

  void finishvertices() {
    loopv(m_delayed_qef) {
      const auto &item = m_delayed_qef[i];
      const auto xyz = item.first;
      const auto plod = item.second.x;
      const auto edgemap = item.second.y;

      // special case where there is no edge, we just put the center for now
      // TODO use proper qef vertex
      if (edgemap == 0) {
        const vec3i topright = xyz + (1<<plod);
        const vec3f pos = (vertex(xyz)+vertex(topright))*0.5f;
        m_pos_buffer.add(pos);
        m_nor_buffer.add(vec3f(1.f,0.f,0.f));
        continue;
      }

      // get the intersection points between the surface and the cube edges
      vec3f p[12], n[12], mass(zero);
      vec3f nor = zero;
      int num = 0;
      loopi(12) {
        if ((edgemap & (1<<i)) == 0) continue;
        const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
        const auto e = getedge(icubev[idx0], icubev[idx1], plod);
        const auto idx = m_edge_index[edge_index(xyz+e.first, e.second)];
        assert(idx != NOINDEX);
        p[num] = m_edges[idx].first+vec3f(e.first);
        n[num] = m_edges[idx].second;
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
      const auto pos = mass + qef::evaluate(matrix, vector, num);
      m_pos_buffer.add(vertex(xyz) + pos*float(1<<plod)*m_cellsize);
      m_nor_buffer.add(abs(normalize(nor)));
    }
  }

  void tesselate() {
    const vec3i org(zero), dim(SUBGRID+4);
    loopxyz(org, dim) {
      const int start_sign = field(xyz) < 0.f ? 1 : 0;
      if (abs(field(xyz)) > 2.f*m_cellsize)
        continue;

      // look at the three edges that start on xyz
      loopi(3) {
        // figure out the edge size
        const auto qaxis0 = axis[(i+1)%3];
        const auto qaxis1 = axis[(i+2)%3];
        const vec3i q[] = {xyz, xyz-qaxis0, xyz-qaxis0-qaxis1, xyz-qaxis1};
        u32 edgelod = 1;
        loopj(4) edgelod = min(edgelod, u32(lod(q[j])));

        // process the edge only when we get the properly aligned origin.
        // otherwise, we would generate the same face more than one
        if (any(ne(xyz&vec3i(edgelod), vec3i(zero))))
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
          const auto idx = qef_index(np);

          if (m_qef_index[idx] == NOINDEX) {
            mcell cell;
            loopk(8) cell[k] = field(np + (icubev[k]<<int(plod)));
            const int edgemap = delayed_qef_vertex(cell, np, plod);

            // point is still valid. we are good to go
            m_qef_index[idx] = m_border_remap.length();
            m_border_remap.add(OUTSIDE);
            m_delayed_qef.add(makepair(np,vec2i(plod,edgemap)));
            STATS_INC(iso_qef_num);
          }
          quad[vertnum++] = m_qef_index[idx];
        }
        const u32 msk = any(eq(xyz,vec3i(zero)))||any(gt(xyz,vec3i(SUBGRID)))?OUTSIDE:0u;
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
    initedge();
    initqef();
    initlod();
    tesselate();
    finishedges();
    finishvertices();

    // stop here if we do not create anything
    if (first_vert == m_pos_buffer.length())
      return;

    m_mp.set(m_pos_buffer, m_nor_buffer, m_idx_buffer, m_cracks, first_vert, first_idx);
    const auto edgenum = m_mp.buildedges();
    m_mp.crease(edgenum);
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

  const csg::node *m_node;
  mesh_processor m_mp;
  vector<float> m_field;
  vector<u8> m_lod;
  vector<vec3f> m_pos_buffer, m_nor_buffer;
  vector<pair<u32,u32>> m_cracks;
  vector<u32> m_idx_buffer;
  vector<u32> m_border_remap;
  vector<u32> m_qef_index;
  vector<u32> m_edge_index;
  vector<pair<vec3f,vec3f>> m_edges;
  vector<pair<vec3i,vec3i>> m_delayed_edges;
  vector<pair<vec3i,vec2i>> m_delayed_qef;
  edgestack *m_stack;
  const octree *m_octree;
  vec3f m_org;
  vec3i m_iorg;
  float m_cellsize, m_mincellsize;
  u32 m_maxlevel, m_level;
};

/*-------------------------------------------------------------------------
 - multi-threaded implementation of the iso surface extraction
 -------------------------------------------------------------------------*/
static THREAD dc_gridbuilder *localbuilder = NULL;
struct jobdata {
  const csg::node *m_csg_node;
  octree::node *m_octree_node;
  octree *m_octree;
  vec3i m_iorg;
  vec3f m_org;
  int m_level;
  int m_maxlevel;
  float m_cellsize;
  float m_mincellsize;
};

struct context {
  INLINE context() : m_mutex(SDL_CreateMutex()) {}
  SDL_mutex *m_mutex;
  vector<jobdata> m_work;
  vector<dc_gridbuilder*> m_builders; // one per thread
};
static context *ctx = NULL;

static const int splus = SUBGRID;
static const int sminus = -1;
static const vec3i nodeneighbors[] = {
  // we share a face with these neighbors
  vec3i(splus,0,0), vec3i(0,splus,0), vec3i(0,0,splus),
  vec3i(sminus,0,0),vec3i(0,sminus,0),vec3i(0,0,sminus),

  // we share an edge with these neighbors
  vec3i(splus,splus,0),  vec3i(splus,0,splus),  vec3i(0,splus,splus),
  vec3i(sminus,sminus,0),vec3i(sminus,0,sminus),vec3i(0,sminus,sminus),
  vec3i(splus,sminus,0), vec3i(sminus,splus,0), vec3i(splus,0,sminus),
  vec3i(sminus,0,splus), vec3i(0,sminus,splus), vec3i(0,splus,sminus)
};
static const int nodeneighbornum = ARRAY_ELEM_NUM(nodeneighbors);

struct mt_builder {
  mt_builder(const csg::node &node, const vec3f &org, float cellsize, u32 dim) :
    m_node(&node), m_org(org),
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

  void build(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    const auto scale = 1.f / float(1<<level);
    const auto cellnum = m_dim >> level;
    const auto org = pos(xyz);

    // XXX fix this horrible code
    const auto len = float(1<<(m_maxlevel-level))*m_cellsize;
    const auto center = org + scale*vec3f(m_half_side_len);
    const vec3f pmin(org-4.f*len);
    const vec3f pmax(org+2.f*scale*m_half_side_len+4.f*len);
    const auto pt = csg::dist(*m_node, center, aabb(pmin,pmax));
    // const auto pt = csg::dist(*m_node, center);
    const auto dist = pt.dist;
    STATS_INC(iso_octree_num);
    STATS_INC(iso_num);
    node.m_level = level;
    if (abs(dist) > m_half_diag_len * scale) {
      node.m_isleaf = node.m_empty = 1;
      return;
    }
    //if (cellnum == SUBGRID || (level == 5 && xyz.x >= 32) || (level == 5 && xyz.y >= 16) || pt.size > len) {
    //if (cellnum == SUBGRID || (level == 5 && xyz.x >= 80) || (level == 5 && xyz.y >= 16)) {
#if USE_NODE_SIZE
    if (cellnum == SUBGRID || pt.size > len) {
#else
    if (cellnum == SUBGRID) {
#endif
      node.m_isleaf = 1;
      return;
    } else {
      node.m_children = NEWAE(octree::node, 8);
      loopi(8) {
        const auto childxyz = xyz+int(cellnum/2)*icubev[i];
        build(node.m_children[i], childxyz, level+1);
      }
      return;
    }
  }

  bool addconstraints(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    const auto lod = m_maxlevel - level;

    // figure out if we need to subdivide more
    if (node.m_isleaf && !node.m_empty) {
      u32 neighborlevel = level;
      loopi(nodeneighbornum) {
        const auto neighborpos = xyz+(nodeneighbors[i]<<int(lod));
        const auto neighbor = m_octree->findleaf(neighborpos);
        if (neighbor.first != NULL && !neighbor.first->m_empty)
          neighborlevel = max(neighbor.second, neighborlevel);
      }

      // we do not match the LOD constraints. we need to subdivide more and
      // recurse
      if (neighborlevel - level > 1) {
        node.m_isleaf = 0;
        node.m_children = NEWAE(octree::node, 8);
        loopi(8) {
          node.m_children[i].m_level = level+1;
          node.m_children[i].m_isleaf = 1;
          node.m_children[i].m_empty = 0;
        }
        loopi(8) {
          const auto childxyz = xyz + int(SUBGRID<<(lod-1)) * icubev[i];
          addconstraints(node.m_children[i], childxyz, level+1);
        }
        return true;
      }
    } else if (!node.m_isleaf) {
      bool anychange = false;
      loopi(8) {
        const auto childxyz = xyz + int(SUBGRID<<(lod-1)) * icubev[i];
        anychange = addconstraints(node.m_children[i], childxyz, level+1) || anychange;
      }
      return anychange;
    }
    return false;
  }

  void preparejobs(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    if (node.m_isleaf && !node.m_empty) {
      jobdata job;
      job.m_octree = m_octree;
      job.m_octree_node = &node;
      job.m_csg_node = m_node;
      job.m_iorg = xyz;
      job.m_level = level;
      job.m_maxlevel = m_maxlevel;
      job.m_cellsize = float(1<<(m_maxlevel-level)) * m_cellsize;
      job.m_mincellsize = m_cellsize;
      job.m_org = pos(xyz);
      ctx->m_work.add(job);
      return;
    } else if (!node.m_isleaf) {
      loopi(8) {
        const auto cellnum = m_dim >> level;
        const auto childxyz = xyz+int(cellnum/2)*icubev[i];
        preparejobs(node.m_children[i], childxyz, level+1);
      }
      return;
    }
  }

  void addconstraints() {
    bool anychange = true;
    while (anychange) anychange = addconstraints(m_octree->m_root);
  }

  octree *m_octree;
  const csg::node *m_node;
  vec3f m_org;
  float m_cellsize, m_half_side_len, m_half_diag_len;
  u32 m_dim, m_maxlevel;
};

struct isotask : public task {
  INLINE isotask(u32 n) : task("isotask", n, 1) {}
  virtual void run(u32 idx) {
    if (localbuilder == NULL) {
      localbuilder = NEWE(dc_gridbuilder);
      SDL_LockMutex(ctx->m_mutex);
      ctx->m_builders.add(localbuilder);
      SDL_UnlockMutex(ctx->m_mutex);
    }
    const auto &job = ctx->m_work[idx];
    localbuilder->m_octree = job.m_octree;
    localbuilder->m_iorg = job.m_iorg;
    localbuilder->m_level = job.m_level;
    localbuilder->m_maxlevel = job.m_maxlevel;
    localbuilder->setcellsize(job.m_cellsize);
    localbuilder->setmincellsize(job.m_mincellsize);
    localbuilder->setnode(job.m_csg_node);
    localbuilder->setorg(job.m_org);
    localbuilder->build(*job.m_octree_node);
  }
};

mesh dc_mesh_mt(const vec3f &org, u32 cellnum, float cellsize, const csg::node &d) {
  octree o(cellnum);
  ctx->m_work.setsize(0);
  loopv(ctx->m_builders) {
    ctx->m_builders[i]->m_pos_buffer.setsize(0);
    ctx->m_builders[i]->m_nor_buffer.setsize(0);
    ctx->m_builders[i]->m_idx_buffer.setsize(0);
  }
  mt_builder r(d, org, cellsize, cellnum);
  r.setoctree(o);
  r.build(o.m_root);
  r.addconstraints();
  r.preparejobs(o.m_root);

  // build the grids in parallel
  ref<task> job = NEW(isotask, ctx->m_work.length());
  job->scheduled();
  job->wait();

  // copy back all local buffers together
  vector<vec3f> posbuf, norbuf;
  vector<u32> idxbuf;
  loopv(ctx->m_builders) {
    const auto &b = *ctx->m_builders[i];
    const auto old = posbuf.length();
    const auto buflen = b.m_pos_buffer.length();
    const auto idxlen = b.m_idx_buffer.length();
    loopj(idxlen) idxbuf.add(old+b.m_idx_buffer[j]);
    loopj(buflen) {
      posbuf.add(b.m_pos_buffer[j]);
      norbuf.add(b.m_nor_buffer[j]);
    }
  }

  const auto p = posbuf.move();
  const auto n = norbuf.move();
  const auto idx = idxbuf.move();
  const auto m = mesh(p.first, n.first, idx.first, p.second, idx.second);
#if !defined(NDEBUG)
  stats();
#endif
  return m;
}

void start() { ctx = NEWE(context); }
void finish() {
  if (ctx == NULL) return;
  loopv(ctx->m_builders) SAFE_DEL(ctx->m_builders[i]);
  DEL(ctx);
}
} /* namespace iso */
} /* namespace q */

