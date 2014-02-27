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

#define SHARP_EDGE_DC 0

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
static const float SHARP_EDGE_THRESHOLD = 0.4f;

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
static const u32 quadorder[] = {0,1,2,3};
static const u32 quadorder_cc[] = {3,2,1,0};
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
static const u32 FIELDDIM = SUBGRID+4;
static const u32 QEFDIM = SUBGRID+2;
static const u32 FIELDNUM = FIELDDIM*FIELDDIM*FIELDDIM;
static const u32 QEFNUM = QEFDIM*QEFDIM*QEFDIM;
static const u32 MAX_NEW_VERT = 8;
static const u32 NOINDEX = ~0x0u;
static const u32 NOTRIANGLE = ~0x0u-1;

struct edgeitem {
  vec3f org, p0, p1;
  float v0, v1;
};

struct edgestack {
  array<edgeitem,64> it;
  array<vec3f,64> p, pos;
  array<float,64> d;
};

/*-------------------------------------------------------------------------
 - spatial segmentation used for iso surface extraction
 -------------------------------------------------------------------------*/
struct octree {
  struct qefpoint {
    vec3f pos;
#if SHARP_EDGE_DC
    int idx:30;
    int sharp:2;
#else
    int idx;
#endif
  };
  struct node {
    INLINE node() : children(NULL), m_level(0), m_isleaf(0), m_empty(0) {}
    ~node() {
      if (m_isleaf)
        SAFE_DELA(qef);
      else
        SAFE_DELA(children);
    }
    union {
      node *children;
      qefpoint *qef;
    };
    u32 m_level:30;
    u32 m_isleaf:1, m_empty:1;
  };

  INLINE octree(u32 dim) : m_dim(dim), m_logdim(ilog2(dim)) {}
  const node *findleaf(vec3i xyz) const {
    if (any(lt(xyz,vec3i(zero))) || any(ge(xyz,vec3i(m_dim)))) return NULL;
    int level = 0;
    const node *node = &m_root;
    for (;;) {
      if (node->m_isleaf) return node;
      assert(m_logdim > u32(level));
      const auto logsize = vec3i(m_logdim-level-1);
      const auto bits = xyz >> logsize;
      const auto child = octreechildmap[bits.x | (bits.y<<1) | (bits.z<<2)];
      assert(all(ge(xyz, icubev[child] << logsize)));
      xyz -= icubev[child] << logsize;
      node = node->children + child;
      ++level;
    }
    assert("unreachable" && false);
    return NULL;
  }

  static const u32 MAX_ITEM_NUM = (1<<15)-1;
  node m_root;
  u32 m_dim, m_logdim;
};

INLINE pair<vec3i,int> getedge(const vec3i &start, const vec3i &end) {
  const auto lower = select(le(start,end), start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, delta.y+2*delta.z);
}

// TODO find less expensive storage for it
struct quad {vec3i index[4];};

/*-------------------------------------------------------------------------
 - iso surface extraction is done here
 -------------------------------------------------------------------------*/
struct dc_gridbuilder {
  dc_gridbuilder() :
    m_node(NULL),
    m_field(FIELDNUM),
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
    const vec3i p = xyz + 1;
    return p.x + (p.y + p.z * QEFDIM) * QEFDIM;
  }
  INLINE u32 field_index(const vec3i &xyz) {
    const vec3i p = xyz + 1;
    return p.x + (p.y + p.z * FIELDDIM) * FIELDDIM;
  }
  INLINE u32 edge_index(const vec3i &start, int edge) {
    const auto p = start + 1;
    const auto offset = p.x + (p.y + p.z * FIELDDIM) * FIELDDIM;
    return offset + edge * FIELDNUM;
  }

  INLINE float &field(const vec3i &xyz) { return m_field[field_index(xyz)]; }

  void initfield() {
    const vec3i org(-1), dim(SUBGRID+3);
    stepxyz(org, dim, vec3i(4)) {
      vec3f pos[64];
      float distance[64];
      const auto p = vertex(sxyz);
      const aabb box = aabb(p-2.f*m_cellsize, p+6.f*m_cellsize);
      int index = 0;
      const auto end = min(sxyz+4,dim);
      loopxyz(sxyz, end) pos[index++] = vertex(xyz);
      assert(index == 64);
      csg::dist(m_node, pos, distance, index, box);
      index = 0;
      loopxyz(sxyz, end) field(xyz) = distance[index++];
      STATS_ADD(iso_num, 64);
      STATS_ADD(iso_grid_num, 64);
    }
  }

  NOINLINE void initedge() {
    m_edge_index.memset(0xff);
    m_edges.setsize(0);
    m_delayed_edges.setsize(0);
  }

  NOINLINE void initqef() {
    m_qefnum = 0;
    m_qef_index.memset(0xff);
    m_delayed_qef.setsize(0);
  }

  int delayed_qef_vertex(const mcell &cell, const vec3i &xyz) {
    int cubeindex = 0;
    loopi(8) if (cell[i] < 0.0f) cubeindex |= 1<<i;
    if (edgetable[cubeindex] == 0)
      return 0;
    loopi(12) {
      if ((edgetable[cubeindex] & (1<<i)) == 0)
        continue;
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto e = getedge(icubev[idx0], icubev[idx1]);
      auto &idx = m_edge_index[edge_index(xyz+e.first, e.second)];
      if (idx == NOINDEX) {
        idx = m_delayed_edges.length();
        m_delayed_edges.add(makepair(xyz, vec3i(idx0, idx1, 0)));
      }
    }
    return edgetable[cubeindex];
  }

  INLINE void falsepos(const csg::node *node, edgestack &stack, int num) {
    assert(num <= 64);
    auto &it = stack.it;
    auto &pos = stack.pos, &p = stack.p;
    auto &d = stack.d;

    loopk(MAX_STEPS) {
      auto box = aabb::empty();
      loopi(num) {
        p[i] = it[i].p1 - it[i].v1 * (it[i].p1-it[i].p0)/(it[i].v1-it[i].v0);
        pos[i] = it[i].org+m_cellsize*p[i];
        box.pmin = min(pos[i], box.pmin);
        box.pmax = max(pos[i], box.pmax);
      }
      box.pmin -= 3.f * m_cellsize;
      box.pmax += 3.f * m_cellsize;
      csg::dist(m_node, &pos[0], &d[0], num, box);
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

      // step 1 - run false position with packets of (up-to) 64 points. we need
      // to be careful FP wise. We ensure here that the position computation is
      // invariant from grids to grids such that neighbor grids will output the
      // exact same result
      const int num = min(64, len-i);
      loopj(num) {
        const auto &e = m_delayed_edges[i+j];
        const auto idx0 = e.second.x, idx1 = e.second.y;
        const auto xyz = e.first;
        const auto edge = getedge(icubev[idx0], icubev[idx1]);
        it[j].org = vertex(xyz+edge.first);
        it[j].p0 = vec3f(icubev[idx0]-edge.first);
        it[j].p1 = vec3f(icubev[idx1]-edge.first);
        it[j].v0 = field(xyz + icubev[idx0]);
        it[j].v1 = field(xyz + icubev[idx1]);
        if (it[j].v1 < 0.f) {
          swap(it[j].p0,it[j].p1);
          swap(it[j].v0,it[j].v1);
        }
      }
      falsepos(m_node, *m_stack, num);

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
          p[4*k]   = it[j+k].org + it[j+k].p0 * m_cellsize;
          p[4*k+1] = p[4*k]-dx;
          p[4*k+2] = p[4*k]-dy;
          p[4*k+3] = p[4*k]-dz;
          box.pmin = min(p[4*k], box.pmin);
          box.pmax = max(p[4*k], box.pmax);
        }
        box.pmin -= 3.f * m_cellsize;
        box.pmax += 3.f * m_cellsize;

        csg::dist(m_node, p, d, 4*subnum, box);
        STATS_ADD(iso_num, 4*subnum);
        STATS_ADD(iso_gradient_num, 4*subnum);

        loopk(subnum) {
          const auto c    = d[4*k+0];
          const auto dfdx = d[4*k+1];
          const auto dfdy = d[4*k+2];
          const auto dfdz = d[4*k+3];
          const auto grad = vec3f(c-dfdx, c-dfdy, c-dfdz);
          const auto n = grad==vec3f(zero) ? vec3f(zero) : normalize(grad);
          const auto p = it[j+k].p0;
          m_edges.add(makepair(p,n));
        }
      }
    }
  }

  void finishvertices(octree::node &node) {
    loopv(m_delayed_qef) {
      const auto &item = m_delayed_qef[i];
      const auto xyz = item.first;
      const auto edgemap = item.second;

      // this vertex does not belong this leaf. skip it. we test all three sign
      if ((xyz.x | xyz.y | xyz.z) >> 31) continue;

      // get the intersection points between the surface and the cube edges
      vec3f p[12], n[12], mass(zero);
      vec3f nor = zero;
      int num = 0;
      loopi(12) {
        if ((edgemap & (1<<i)) == 0) continue;
        const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
        const auto e = getedge(icubev[idx0], icubev[idx1]);
        const auto idx = m_edge_index[edge_index(xyz+e.first, e.second)];
        assert(idx != NOINDEX);
        p[num] = m_edges[idx].first+vec3f(e.first);
        n[num] = m_edges[idx].second;
        nor += n[num];
        mass += p[num++];
      }
      mass /= float(num);

#if SHARP_EDGE_DC
      // figure out if the point is on a feature
      const auto normean = nor / float(num);
      vec3f var(zero);
      loopi(num) var += (normean-n[i])*(normean-n[i]);
      var /= float(num);
      const bool sharp = any(gt(var,0.5f));
#endif

      // compute the QEF minimizing point
      double matrix[12][3];
      double vector[12];
      loopi(num) {
        loopj(3) matrix[i][j] = double(n[i][j]);
        const auto d = p[i] - mass;
        vector[i] = double(dot(n[i],d));
      }
      const auto pos = mass + qef::evaluate(matrix, vector, num);

      // XXX test should go away at the end
      const auto qef = vertex(xyz) + pos*m_cellsize;;
      if (all(ge(xyz,vec3i(zero))) && all(lt(xyz,vec3i(SUBGRID)))) {
        const auto idx = xyz.x+SUBGRID*(xyz.y+xyz.z*SUBGRID);
        node.qef[idx].pos = qef;
        node.qef[idx].idx = -1;
#if SHARP_EDGE_DC
        node.qef[idx].sharp = sharp?1:0;
#endif
      }
    }
  }

  void tesselate() {
    const vec3i org(zero), dim(SUBGRID+1);
    loopxyz(org, dim) {
      const int start_sign = field(xyz) < 0.f ? 1 : 0;
      if (abs(field(xyz)) > 2.f*m_cellsize) continue;

      // some quads belong to our neighbor. we will not push them but we need to
      // compute their vertices such that our neighbor can output these quads
      const auto outside = any(eq(xyz,0));

      // look at the three edges that start on xyz
      loopi(3) {

        // is it an actual edge?
        const auto delta = axis[i];
        const int end_sign = field(xyz+delta) < 0.f ? 1 : 0;
        if (start_sign == end_sign) continue;

        // we found one edge. we output one quad for it
        const auto axis0 = axis[(i+1)%3];
        const auto axis1 = axis[(i+2)%3];
        const vec3i p[] = {xyz, xyz-axis0, xyz-axis0-axis1, xyz-axis1};
        loopj(4) {
          const auto np = p[j];
          const auto idx = qef_index(np);
          if (m_qef_index[idx] == NOINDEX) {
            mcell cell;
            loopk(8) cell[k] = field(np+icubev[k]);
            m_qef_index[idx] = m_qefnum++;
            m_delayed_qef.add(makepair(np,delayed_qef_vertex(cell, np)));
            STATS_INC(iso_qef_num);
          }
        }

        // we must use a more compact storage for it
        if (outside) continue;
        const auto qor = start_sign==1 ? quadorder : quadorder_cc;
        const quad q = {m_iorg+p[qor[0]], m_iorg+p[qor[1]], m_iorg+p[qor[2]], m_iorg+p[qor[3]]};
        m_quad_buffer.add(q);
      }
    }
  }

  void build(octree::node &node) {
    initfield();
    initedge();
    initqef();
    tesselate();
    finishedges();
    finishvertices(node);
  }

  const csg::node *m_node;
  vector<float> m_field;
  vector<quad> m_quad_buffer;
  vector<u32> m_qef_index;
  vector<u32> m_edge_index;
  vector<pair<vec3f,vec3f>> m_edges;
  vector<pair<vec3i,vec3i>> m_delayed_edges;
  vector<pair<vec3i,int>> m_delayed_qef;
  edgestack *m_stack;
  const octree *m_octree;
  vec3f m_org;
  vec3i m_iorg;
  float m_cellsize, m_mincellsize;
  u32 m_qefnum;
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
  vector<dc_gridbuilder*> m_builders;
};
static context *ctx = NULL;

struct mt_builder {
  mt_builder(const csg::node &node, const vec3f &org, float cellsize, u32 dim) :
    m_node(&node), m_org(org),
    m_cellsize(cellsize),
    m_dim(dim)
  {
    assert(ispoweroftwo(dim) && dim % SUBGRID == 0);
    m_maxlevel = ilog2(dim / SUBGRID);
  }
  INLINE vec3f pos(const vec3i &xyz) { return m_org+m_cellsize*vec3f(xyz); }
  INLINE void setoctree(octree &o) {m_octree=&o;}

  void build(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    node.m_level = level;

    // bounding box of this octree cell
    const auto lod = m_maxlevel - level;
    const vec3f pmin = pos(xyz - int(4<<lod));
    const vec3f pmax = pos(xyz + int((SUBGRID+4)<<lod));

    // center of the box where to evaluate the distance field
    const auto cellnum = int(m_dim >> level);
    const auto icenter = xyz + cellnum/2;
    const auto center = pos(icenter);
    const auto dist = csg::dist(m_node, center, aabb(pmin,pmax));
    STATS_INC(iso_octree_num);
    STATS_INC(iso_num);
    if (abs(dist) > sqrt(3.f) * m_cellsize * float(cellnum/2)) {
      node.m_isleaf = node.m_empty = 1;
      return;
    }
    if (cellnum == SUBGRID) {
      node.qef = NEWAE(octree::qefpoint, SUBGRID*SUBGRID*SUBGRID);
      node.m_isleaf = 1;
    } else {
      node.children = NEWAE(octree::node, 8);
      loopi(8) {
        const auto childxyz = xyz+cellnum*icubev[i]/2;
        build(node.children[i], childxyz, level+1);
      }
    }
  }

  void preparejobs(octree::node &node, const vec3i &xyz = vec3i(zero)) {
    if (node.m_isleaf && !node.m_empty) {
      jobdata job;
      job.m_octree = m_octree;
      job.m_octree_node = &node;
      job.m_csg_node = m_node;
      job.m_iorg = xyz;
      job.m_maxlevel = m_maxlevel;
      job.m_level = node.m_level;
      job.m_cellsize = float(1<<(m_maxlevel-node.m_level)) * m_cellsize;
      job.m_mincellsize = m_cellsize;
      job.m_org = pos(xyz);
      ctx->m_work.add(job);
      return;
    } else if (!node.m_isleaf) {
      loopi(8) {
        const auto cellnum = m_dim >> node.m_level;
        const auto childxyz = xyz+int(cellnum/2)*icubev[i];
        preparejobs(node.children[i], childxyz);
      }
      return;
    }
  }

  octree *m_octree;
  const csg::node *m_node;
  vec3f m_org;
  float m_cellsize;
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
    localbuilder->m_level = job.m_octree_node->m_level;
    localbuilder->m_maxlevel = job.m_maxlevel;
    localbuilder->setcellsize(job.m_cellsize);
    localbuilder->setmincellsize(job.m_mincellsize);
    localbuilder->setnode(job.m_csg_node);
    localbuilder->setorg(job.m_org);
    localbuilder->build(*job.m_octree_node);
  }
};

// we have to choose between this two meshes and take the convex one
struct quadmesh { int tri[2][3]; };
static const quadmesh qmesh[2] = {{{{0,1,2},{0,2,3}}},{{{0,1,3},{3,1,2}}}};

// test first configuration. If ok, take it, otherwise take the other one
#if SHARP_EDGE_DC
INLINE const quadmesh &findbestmesh(octree::qefpoint **pt) {
  int sharpnum = 0, sharpindex = 0;
  loopi(4) if (pt[i]->sharp) { ++sharpnum; sharpindex = i; }
  if (sharpnum == 0)
    return qmesh[0];
  else if (sharpnum == 1) {
    if (sharpindex == 0 || sharpindex == 2) return qmesh[0];
    return qmesh[1];
  } else if (pt[0]->sharp && pt[2]->sharp)
    return qmesh[0];
  else
    return qmesh[1];
}
#else
INLINE const quadmesh &findbestmesh(octree::qefpoint **pt) {return qmesh[0];}
#endif

static mesh buildmesh(octree &o) {

  // build the buffers here
  vector<vec3f> posbuf, norbuf;
  vector<u32> idxbuf;
  vector<pair<int,int>> vertlist;

  // merge all builder vertex buffers
  loopv(ctx->m_builders) {
    const auto &b = *ctx->m_builders[i];
    const auto quadlen = b.m_quad_buffer.length();

    // loop over all quads. build triangle mesh on the fly merging normals when
    // close enough
    loopj(quadlen) {
      // get four points
      const auto &q = b.m_quad_buffer[j];
      octree::qefpoint *pt[4];
      loopk(4) {
        const auto *leaf = o.findleaf(q.index[k]);
        const auto idx = q.index[k] % vec3i(SUBGRID);
        const auto qefidx = idx.x+SUBGRID*(idx.y+SUBGRID*idx.z);
        pt[k] = leaf->qef+qefidx;
      }

      // get the right convex configuration
      const auto tri = findbestmesh(pt).tri;

      // compute triangle normals and append points in the vertex buffer and the
      // index buffer
      loopk(2) {
        const auto t = tri[k];
        const auto edge0 = pt[t[2]]->pos-pt[t[0]]->pos;
        const auto edge1 = pt[t[2]]->pos-pt[t[1]]->pos;
        const auto dir = cross(edge0, edge1);
        const auto len2 = length2(dir);
        if (len2 == 0.f) continue;
        const auto nor = dir/sqrt(len2);
        loopl(3) {
          auto qef = pt[t[l]];
          int vertidx = -1, curridx = qef->idx;

          // try to find a good candidate with a normal close to ours
          while (curridx != -1) {
            auto const curr = vertlist[curridx];
            vertidx = curr.first;
            auto const cosangle = dot(nor, normalize(norbuf[vertidx]));
            if (cosangle > SHARP_EDGE_THRESHOLD) break;
            curridx = curr.second;
          }

          // create a brand new vertex here
          if (curridx == -1) {
            const auto head = qef->idx;
            qef->idx = vertlist.length();
            vertidx = posbuf.length();
            vertlist.add(makepair(vertidx,head));
            posbuf.add(qef->pos);
            norbuf.add(dir);
          }
          // update the vertex normal
          else
            norbuf[vertidx] += dir;
          idxbuf.add(vertidx);
        }
      }
    }
  }

  // normalize all vertex normals now
  loopv(norbuf) norbuf[i] = normalize(norbuf[i]);

  const auto p = posbuf.move();
  const auto n = norbuf.move();
  const auto idx = idxbuf.move();
  return mesh(p.first, n.first, idx.first, p.second, idx.second);
}

mesh dc_mesh_mt(const vec3f &org, u32 cellnum, float cellsize, const csg::node &d) {
  octree o(cellnum);
  ctx->m_work.setsize(0);
  mt_builder r(d, org, cellsize, cellnum);
  r.setoctree(o);
  r.build(o.m_root);
  r.preparejobs(o.m_root);

  // build the grids in parallel
  ref<task> job = NEW(isotask, ctx->m_work.length());
  job->scheduled();
  job->wait();

#if !defined(NDEBUG)
  stats();
#endif
  return buildmesh(o);
}

void start() { ctx = NEWE(context); }
void finish() {
  if (ctx == NULL) return;
  loopv(ctx->m_builders) SAFE_DEL(ctx->m_builders[i]);
  DEL(ctx);
}
} /* namespace iso */
} /* namespace q */

