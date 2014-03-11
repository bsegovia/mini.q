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
STATS(iso_edgepos_num);
STATS(iso_edge_num);
STATS(iso_gradient_num);
STATS(iso_grid_num);
STATS(iso_octree_num);
STATS(iso_qef_num);
STATS(iso_edgepos);

#define SHARP_EDGE_DC 0

#if !defined(NDEBUG)
static void stats() {
  STATS_OUT(iso_num);
  STATS_OUT(iso_qef_num);
  STATS_OUT(iso_edge_num);
  STATS_RATIO(iso_edgepos_num, iso_edge_num);
  STATS_RATIO(iso_edgepos_num, iso_num);
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
static const int MAX_STEPS = 8;
static const float SHARP_EDGE_THRESHOLD = 0.5f;
static const float MIN_EDGE_FACTOR = 1.f/8.f;
static const float MIN_TRIANGLE_AREA = 1e-6f;

INLINE pair<vec3i,u32> edge(vec3i start, vec3i end) {
  const auto lower = select(lt(start,end), start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, u32(delta.y+2*delta.z));
}
struct fielditem {
  INLINE fielditem(float d, u32 m) : d(d), m(m) {}
  INLINE fielditem() {}
  float d;
  u32 m;
};
typedef fielditem mcell[8];

static const vec3i axis[] = {vec3i(1,0,0), vec3i(0,1,0), vec3i(0,0,1)};
static const u32 quadorder[] = {0,1,2,3};
static const u32 quadorder_cc[] = {3,2,1,0};
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

static const u32 SUBGRID = 16;
static const u32 FIELDDIM = SUBGRID+4;
static const u32 QEFDIM = SUBGRID+2;
static const u32 FIELDNUM = FIELDDIM*FIELDDIM*FIELDDIM;
static const u32 QEFNUM = QEFDIM*QEFDIM*QEFDIM;
static const u32 NOINDEX = ~0x0u;

struct edgeitem {
  vec3f org, p0, p1;
  float v0, v1;
  u32 m0, m1;
};

struct edgestack {
  array<edgeitem,64> it;
  array<vec3f,64> p, pos;
  array<float,64> d, nd;
  array<u32,64> m;
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
struct quad {vec3i index[4]; u32 matindex;};

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
    return vec3f(ipos)*m_cellsize;
  }
  INLINE void setoctree(const octree &o) { m_octree = &o; }
  INLINE void setorg(const vec3f &org) { m_org = org; }
  INLINE void setcellsize(float size) { m_cellsize = size; }
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

  INLINE fielditem &field(const vec3i &xyz) {return m_field[field_index(xyz)];}

  void initfield() {
    const vec3i org(-1), dim(SUBGRID+3);
    stepxyz(org, dim, vec3i(4)) {
      // use edgestack here
      vec3f pos[64];
      float d[64];
      u32 m[64];
      const auto p = vertex(sxyz);
      const aabb box = aabb(p-2.f*m_cellsize, p+6.f*m_cellsize);
      int index = 0;
      const auto end = min(sxyz+4,dim);
      loopxyz(sxyz, end) pos[index++] = vertex(xyz);
      assert(index == 64);
      csg::dist(m_node, pos, NULL, d, m, index, box);
      index = 0;
#if !defined(NDEBUG)
      loopi(64) assert(d[i] <= 0.f || m[i] == csg::MAT_AIR_INDEX);
      loopi(64) assert(d[i] >= 0.f || m[i] != csg::MAT_AIR_INDEX);
#endif
      loopxyz(sxyz, end) {
        field(xyz) = fielditem(d[index], m[index]);
        ++index;
      }
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
    int edgemap = 0;
    loopi(12) {
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto m0 = cell[idx0].m, m1 = cell[idx1].m;
      if (m0 == m1) continue;
      const auto e = getedge(icubev[idx0], icubev[idx1]);
      auto &idx = m_edge_index[edge_index(xyz+e.first, e.second)];
      if (idx == NOINDEX) {
        idx = m_delayed_edges.length();
        m_delayed_edges.add(makepair(xyz, vec4i(idx0, idx1, m0, m1)));
      }
      edgemap |= (1<<i);
    }
    return edgemap;
  }

  INLINE void edgepos(const csg::node *node, edgestack &stack, int num) {
    assert(num <= 64);
    auto &it = stack.it;
    auto &pos = stack.pos, &p = stack.p;
    auto &d = stack.d;
    auto &m = stack.m;

    loopk(MAX_STEPS) {
      auto box = aabb::empty();
      loopi(num) {
        const auto samesign = it[i].v1*it[i].v0 > 0.f || it[i].v1==it[i].v0;
        if (!samesign)
          p[i] = it[i].p1 - it[i].v1 * (it[i].p1-it[i].p0)/(it[i].v1-it[i].v0);
        else
          p[i] = (it[i].p0 + it[i].p1) * 0.5f;
        pos[i] = it[i].org+m_cellsize*p[i];
        box.pmin = min(pos[i], box.pmin);
        box.pmax = max(pos[i], box.pmax);
      }
      box.pmin -= 3.f * m_cellsize;
      box.pmax += 3.f * m_cellsize;
      csg::dist(m_node, &pos[0], NULL, &d[0], &m[0], num, box);
      if (k != MAX_STEPS-1) {
        loopi(num) {
          assert(!isnan(d[i]));
          if (m[i] == it[i].m0) {
            it[i].p0 = p[i];
            it[i].v0 = d[i];
          } else {
            it[i].p1 = p[i];
            it[i].v1 = d[i];
          }
        }
      } else {
        loopi(num) {
          it[i].p0 = p[i];
#if !defined(NDEBUG)
          assert(!isnan(p[i].x)&&!isnan(p[i].y)&&!isnan(p[i].z));
          assert(!isinf(p[i].x)&&!isinf(p[i].y)&&!isinf(p[i].z));
#endif
        }
      }
    }
    STATS_ADD(iso_edgepos_num, num*MAX_STEPS);
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
        it[j].v0 = field(xyz + icubev[idx0]).d;
        it[j].v1 = field(xyz + icubev[idx1]).d;
        it[j].m0 = e.second.z;
        it[j].m1 = e.second.w;
        if (it[j].v1 < 0.f) {
          swap(it[j].p0,it[j].p1);
          swap(it[j].v0,it[j].v1);
          swap(it[j].m0,it[j].m1);
        }
      }
      edgepos(m_node, *m_stack, num);

      // step 2 - compute normals for each point using packets of 16x4 points
      const auto dx = vec3f(DEFAULT_GRAD_STEP, 0.f, 0.f);
      const auto dy = vec3f(0.f, DEFAULT_GRAD_STEP, 0.f);
      const auto dz = vec3f(0.f, 0.f, DEFAULT_GRAD_STEP);
      for (int j = 0; j < num; j += 16) {
        const int subnum = min(num-j, 16);
        auto p = &m_stack->p[0];
        auto d = &m_stack->d[0];
        auto m = &m_stack->m[0];
        auto nd = &m_stack->nd[0];
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

        loopk(4*subnum) {
          const auto m0 = it[j+k/4].m0;
          const auto m1 = it[j+k/4].m1;
          bool const solidsolid = m0 != csg::MAT_AIR_INDEX && m1 != csg::MAT_AIR_INDEX;
          nd[k] = solidsolid ? m_cellsize : 0.f;
        }
        csg::dist(m_node, p, nd, d, m, 4*subnum, box);
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
      if ((xyz.x|xyz.y|xyz.z)>>31) continue;

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
      const auto startfield = field(xyz);
      const auto startsign = startfield.d < 0.f ? 1 : 0;
      if (abs(field(xyz).d) > 2.f*m_cellsize) continue;

      // some quads belong to our neighbor. we will not push them but we need to
      // compute their vertices such that our neighbor can output these quads
      const auto outside = any(eq(xyz,0));

      // look at the three edges that start on xyz
      loopi(3) {

        // is it an actual edge?
        const auto end = xyz+axis[i];
        const auto endfield = field(end);
        const int endsign = endfield.d < 0.f ? 1 : 0;
        if (startsign == endsign) continue;

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
            m_delayed_qef.add(makepair(np, delayed_qef_vertex(cell, np)));
            STATS_INC(iso_qef_num);
          }
        }

        // we must use a more compact storage for it
        if (outside) continue;
        const auto qor = startsign==1 ? quadorder : quadorder_cc;
        const quad q = {
          {m_iorg+p[qor[0]],m_iorg+p[qor[1]],m_iorg+p[qor[2]],m_iorg+p[qor[3]]},
          max(startfield.m, endfield.m)
        };
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
  vector<fielditem> m_field;
  vector<quad> m_quad_buffer;
  vector<u32> m_qef_index;
  vector<u32> m_edge_index;
  vector<pair<vec3f,vec3f>> m_edges;
  vector<pair<vec3i,vec4i>> m_delayed_edges;
  vector<pair<vec3i,int>> m_delayed_qef;
  edgestack *m_stack;
  const octree *m_octree;
  vec3f m_org;
  vec3i m_iorg;
  float m_cellsize;
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

/*-------------------------------------------------------------------------
 - build a regular "to-process" mesh from the qef points and quads
 -------------------------------------------------------------------------*/
struct procmesh {
  vector<vec3f> pos, nor;
  vector<u32> idx, mat;
};

static void buildmesh(const octree &o, procmesh &m) {

  // merge all builder vertex buffers
  loopv(ctx->m_builders) {
    const auto &b = *ctx->m_builders[i];
    const auto quadlen = b.m_quad_buffer.length();

    // loop over all quads and build triangle mesh
    loopj(quadlen) {

      // get four points
      const auto &q = b.m_quad_buffer[j];
      const auto quadmat = q.matindex;
      octree::qefpoint *pt[4];
      loopk(4) {
        const auto leaf = o.findleaf(q.index[k]);
        assert(leaf != NULL && leaf->qef != NULL);
        const auto idx = q.index[k] % vec3i(SUBGRID);
        const auto qefidx = idx.x+SUBGRID*(idx.y+SUBGRID*idx.z);
        pt[k] = leaf->qef+qefidx;
      }

      // get the right convex configuration
      const auto tri = findbestmesh(pt).tri;

      // append positions in the vertex buffer and the index buffer
      loopk(2) {
        const auto t = tri[k];
        m.mat.add(quadmat);
        loopl(3) {
          const auto qef = pt[t[l]];
          if (qef->idx == -1) {
            qef->idx = m.pos.length();
            m.pos.add(qef->pos);
          }
          m.idx.add(qef->idx);
        }
      }
    }
  }
}

/*-------------------------------------------------------------------------
 - run mesh decimation on a regular "to-process" mesh
 -------------------------------------------------------------------------*/
struct qem {
  INLINE qem() {ZERO(this); next=-1;}
  INLINE qem(vec3f v0, vec3f v1, vec3f v2) : timestamp(0), next(-1) {
    build(cross(v0-v1,v0-v2), v0);
  }
  INLINE qem(vec3f d, vec3f p) : timestamp(0), next(-1) { build(d,p); }
  INLINE void build(const vec3f &d, const vec3f &p) {
    const auto len = length(d);
    if (len != 0.f) {
      const auto n = vec3d(d/len);
      const auto d = -dot(n, vec3d(p));
      const auto a = n.x, b = n.y, c = n.z;
      aa = a*a; bb = b*b; cc = c*c; dd = d*d;
      ab = a*b; ac = a*c; ad = a*d;
      bc = b*c; bd = b*d;
      cd = c*d;
    } else {
      aa = bb = cc = dd = 0.0;
      ab = ac = ad = 0.0;
      bc = bd = 0.0;
      cd = 0.0;
    }
  }
  INLINE qem operator+= (const qem &q) {
    aa+=q.aa; bb+=q.bb; cc+=q.cc; dd+=q.dd;
    ab+=q.ab; ac+=q.ac; ad+=q.ad;
    bc+=q.bc; bd+=q.bd;
    cd+=q.cd;
    return *this;
  }
  double error(const vec3f &v) const {
    const auto x = double(v.x), y = double(v.y), z = double(v.z);
    const auto err = x*x*aa + 2.0*x*y*ab + 2.0*x*z*ac + 2.0*x*ad +
                     y*y*bb + 2.0*y*z*bc + 2.0*y*bd
                            + z*z*cc     + 2.0*z*cd + dd;
    assert(err > -1e-3);
    return max(err,0.0);
  }
  double aa, bb, cc, dd;
  double ab, ac, ad;
  double bc, bd;
  double cd;
  int timestamp, next;
};

INLINE qem operator+ (const qem &q0, const qem &q1) {
  qem q = q0;
  q += q1;
  return q;
}

struct qemedge {
  INLINE qemedge() {}
  INLINE qemedge(int i0, int i1, int mat) : mat(mat), best(0), num(1) {
    idx[0] = i0;
    idx[1] = i1;
    timestamp[0] = timestamp[1] = 0;
  }
  int idx[2];
  int timestamp[2];
  int mat;
  int best:1;
  int num:31;
};

struct qemheapitem {
  double cost;
  int idx;
};
} /* namespace iso */
template <>
struct heapscore<iso::qemheapitem> {
  static INLINE double get(const iso::qemheapitem &n) {return n.cost;}
};

namespace iso {

static void extraplane(const procmesh &pm, const qemedge &edge, int tri, qem &q0, qem &q1) {
  // need to find which vertex is the third one
  const auto t = &pm.idx[3*tri];
  const auto i0 = edge.idx[0], i1 = edge.idx[1];
  int i2 = t[2];
  loopi(2) if (int(t[i])!=i0 && int(t[i])!=i1) i2=t[i];

  // now we build plane that contains the edge and that is perpendicular to the
  // triangle normal
  const auto &p = pm.pos;
  const auto n0 = cross(p[i2]-p[i0], p[i1]-p[i0]);
  const auto d = normalize(cross(n0, p[i1]-p[i0]));
  const auto q = qem(d,p[i0]);
  q0 += q;
  q1 += q;
}

static void buildedges(const procmesh &pm, vector<qemedge> &e, vector<qem> &v) {

  // maintain a list of edges per triangle
  const auto vertnum = pm.pos.length(), trinum = pm.idx.length()/3;
  vector<int> vlist(pm.idx.length());
  loopi(vertnum) vlist[i] = -1;
  int firstedge = vertnum;

  // first append all edges with i0 < i1
  loopi(trinum) {
    const auto mat = pm.mat[i];
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 < i1) {
        vlist[firstedge++] = vlist[i0];
        vlist[i0] = e.length();
        e.add(qemedge(i0,i1,mat));
      }
      i0 = i1;
    }
  }

  // with i1 < i0, we now handle border and multi-material edges
  auto nextedge = &vlist[0] + vertnum;
  loopi(trinum) {
    const auto mat = pm.mat[i];
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 > i1) {
        int idx = vlist[i1];
        for (; idx != -1; idx = nextedge[idx]) {
          const auto &edge = e[idx];
          if (edge.idx[1] == i0) break;
        }

        // border edge
        if (idx == -1) {
          vlist[firstedge++] = vlist[i1];
          vlist[i1] = e.length();
          e.add(qemedge(i1,i0,mat));
          extraplane(pm, e.last(), i, v[i0], v[i1]);
        }
        else {
          auto &edge = e[idx];
          edge.num += 1;

          // is it a multi-material edge? if so, add a plane to separate both
          // materials
          if (u32(edge.mat) != mat) extraplane(pm, e.last(), i, v[i0], v[i1]);
        }
      }
      i0 = i1;
    }
  }

  // loop again over all triangles and add planes for border edges with i0 < i1
  loopi(trinum) {
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 < i1) {
        int idx = vlist[i1];
        for (; idx != -1; idx = nextedge[idx]) {
          const auto &edge = e[idx];
          if (edge.idx[1] == i0) break;
        }
        assert(e[idx].num >= 1);
        if (e[idx].num == 1) extraplane(pm, e.last(), i, v[i0], v[i1]);
      }
      i0 = i1;
    }
  }
}

INLINE pair<double,int>
findbest(const qem &q0, const qem &q1, const vec3f &p0, const vec3f &p1) {
  const auto q = q0 + q1;
  const auto error0 = q.error(p0), error1 = q.error(p1);
  return error0 < error1 ? makepair(error0, 0) : makepair(error1, 1);
}

static void buildheap(const vector<vec3f> &p,
                      const vector<qem> &v,
                      vector<qemedge> &e,
                      vector<qemheapitem> &h)
{
  loopv(e) {
    const auto idx0 = e[i].idx[0], idx1 = e[i].idx[1];
    const auto &p0 = p[idx0], &p1 = p[idx1];
    const auto &q0 = v[idx0], &q1 = v[idx1];
    const auto best = findbest(q0,q1,p0,p1);
    e[i].best = best.second;
    h[i].cost = best.first;
    h[i].idx = i;
  }
  h.buildheap();
}

INLINE int uncollapsedidx(const vector<qem> &v, int idx) {
  while (v[idx].next != -1) idx = v[idx].next;
  return idx;
}

typedef vector<pair<int,int>> collapselist;

INLINE void merge(const qemedge &edge, collapselist &collapses,
                  int idx0, int idx1, qem &q0, qem &q1)
{
  const auto timestamp0 = q0.timestamp, timestamp1 = q1.timestamp;
  const auto q = q0+q1;
  const auto newstamp = max(timestamp0, timestamp1);
  assert(timestamp0==edge.timestamp[0] && timestamp1==edge.timestamp[1]);
  if (edge.best == 0) {
    q0 = q;
    q1.next = idx0;
    collapses.add(makepair(idx1, idx0));
  } else {
    q1 = q;
    q0.next = idx1;
    collapses.add(makepair(idx0, idx1));
  }
  q0.timestamp = q1.timestamp = newstamp+1;
}

static void decimatemesh(procmesh &pm,
                         vector<qemheapitem> &heap,
                         vector<qem> &vqem,
                         vector<qemedge> &eqem,
                         int toremove,
                         float edgeminlen)
{
  collapselist collapses;
  assert(toremove <= eqem.length());
  bool anychange = true;

  // first we just remove all edges smaller than the given threshold. we iterate
  // until there is nothing left to remove
  while (anychange) {
    anychange = false;
    loopv(eqem) {
      auto &edge = eqem[i];
      const auto idx0 = uncollapsedidx(vqem, edge.idx[0]);
      const auto idx1 = uncollapsedidx(vqem, edge.idx[1]);
      const auto &p0 = pm.pos[idx0];
      const auto &p1 = pm.pos[idx1];

      // we do not care about this edge if already folded or too big
      if (idx0 == idx1 || distance(p0,p1) >= edgeminlen) continue;

      // need to update it properly here
      auto &q0 = vqem[idx0], &q1 = vqem[idx1];
      if (q0.timestamp != edge.timestamp[0] || q1.timestamp != edge.timestamp[1]) {
        edge.best = findbest(q0,q1,p0,p1).second;
        edge.timestamp[0] = q0.timestamp;
        edge.timestamp[1] = q1.timestamp;
      }

      // now we can safely merge it
      merge(edge, collapses, idx0, idx1, vqem[idx0], vqem[idx1]);
      --toremove;
      anychange = true;
    }
  }

#if 0
  // stop when we got enough collapses
  while (toremove > 0) {
    const auto item = heap.removeheap();
    auto &edge = eqem[item.idx];
    const auto idx0 = uncollapsedidx(vqem, edge.idx[0]);
    const auto idx1 = uncollapsedidx(vqem, edge.idx[1]);

    // already removed. we ignore it
    if (idx0 == idx1) {
      --toremove;
      continue;
    }

    // try to remove it. the cost function needs to be properly updated
    const auto &p0 = pm.pos[idx0], &p1 = pm.pos[idx1];
    auto &q0 = vqem[idx0], &q1 = vqem[idx1];
    assert(q0.timestamp >= edge.timestamp[0]);
    assert(q1.timestamp >= edge.timestamp[1]);

    // edge is upto date. we can safely remove it
    const auto timestamp0 = q0.timestamp, timestamp1 = q1.timestamp;
    if (timestamp0 == edge.timestamp[0] && timestamp1 == edge.timestamp[1]) {
      merge(edge, collapses, idx0, idx1, q0, q1);
#if 0
      const auto q = q0+q1;
      const auto newstamp = max(timestamp0, timestamp1);
      if (edge.best == 0) {
        q0 = q;
        q1.next = idx0;
        collapses.add(makepair(idx1, idx0));
      } else {
        q1 = q;
        q0.next = idx1;
        collapses.add(makepair(idx0, idx1));
      }
      q0.timestamp = q1.timestamp = newstamp+1;
#endif
      --toremove;
      continue;
    }

    // edge is too old. we need to update its cost and reinsert it
    const auto best = findbest(q0,q1,p0,p1);
    const qemheapitem newitem = {best.first, item.idx};
    edge.best = best.second;
    edge.timestamp[0] = timestamp0;
    edge.timestamp[1] = timestamp1;
    edge.idx[0] = idx0;
    edge.idx[1] = idx1;
    heap.addheap(newitem);
  }
#endif

  // we play the collapse sequence to get the proper vertex mapping
  vector<int> mapping(pm.pos.length());
  loopv(mapping) mapping[i] = i;
  loopvrev(collapses) mapping[collapses[i].first] = mapping[collapses[i].second];

  // update triangles indices
  loopv(pm.idx) pm.idx[i] = mapping[pm.idx[i]];

  // now, we remove unused vertices and degenerated triangles
  loopv(mapping) mapping[i] = -1;
  vector<u32> newidx, newmat;
  const auto trinum = pm.idx.length()/3;
  auto vertnum = 0;
  loopi(trinum) {
    const auto i0 = pm.idx[3*i+0];
    const auto i1 = pm.idx[3*i+1];
    const auto i2 = pm.idx[3*i+2];
    if (i0 == i1 || i1 == i2 || i2 == i0)
      continue;
    if (mapping[i0] == -1) mapping[i0] = vertnum++;
    if (mapping[i1] == -1) mapping[i1] = vertnum++;
    if (mapping[i2] == -1) mapping[i2] = vertnum++;
    newmat.add(pm.mat[i]);
    newidx.add(mapping[i0]);
    newidx.add(mapping[i1]);
    newidx.add(mapping[i2]);
  }
  newidx.moveto(pm.idx);
  newmat.moveto(pm.mat);

  // compact vertex buffer
  vector<vec3f> newpos(vertnum);
  loopv(mapping) if (mapping[i] != -1) newpos[mapping[i]] = pm.pos[i];
  newpos.moveto(pm.pos);
}

static void buildqem(procmesh &pm, vector<qem> &v) {
  loopi(pm.idx.length()/3) {
    const auto i0 = pm.idx[3*i+0];
    const auto i1 = pm.idx[3*i+1];
    const auto i2 = pm.idx[3*i+2];
    const auto v0 = pm.pos[i0];
    const auto v1 = pm.pos[i1];
    const auto v2 = pm.pos[i2];
    const qem q(v0,v1,v2);
    v[i0] += q;
    v[i1] += q;
    v[i2] += q;
  }
}

static void decimatemesh(procmesh &pm, float cellsize) {

  // we go over all triangles and build all vertex qem
  vector<qem> vqem(pm.pos.length());
  buildqem(pm, vqem);

  // build the list of edges. append extra planes when needed (border and
  // multi-material edges)
  vector<qemedge> eqem;
  buildedges(pm, eqem, vqem);

  // evaluate the cost for each edge and build the heap
  vector<qemheapitem> heap(eqem.length());
  buildheap(pm.pos, vqem, eqem, heap);

  // decimate the mesh using quadric error functions
  const auto minlen = cellsize*MIN_EDGE_FACTOR;;
  decimatemesh(pm, heap, vqem, eqem, eqem.length()*5/10, minlen);
}

/*-------------------------------------------------------------------------
 - sharpen mesh i.e. duplicate sharp points and compute vertex normals
 -------------------------------------------------------------------------*/
static void sharpenmesh(procmesh &pm) {
  const auto trinum = pm.idx.length()/3;
  vector<int> vertlist(pm.pos.length());
  vector<vec3f> newnor(pm.pos.length());
  vector<u32> newidx, newmat;

  // init the chain list
  loopv(vertlist) vertlist[i] = i;

#if !defined(NDEBUG)
  loopv(newnor) newnor[i] = vec3f(zero);
#endif

  // go over all triangles and initialize vertex normals
  loopi(trinum) {
    const auto t = &pm.idx[3*i];
    const auto edge0 = pm.pos[t[2]]-pm.pos[t[0]];
    const auto edge1 = pm.pos[t[2]]-pm.pos[t[1]];

    // zero sized edge are not possible since we got rid of them...
    assert(length(edge0) != 0.f && length(edge1) != 0.f);

    // ...but colinear edges are still possible
    const auto dir = cross(edge0, edge1);
    const auto len2 = length2(dir);
    if (len2 < MIN_TRIANGLE_AREA) continue;
    const auto nor = dir*rsqrt(len2);
    loopj(3) {
      auto idx = int(t[j]);
      if (vertlist[idx] == idx) {
        vertlist[idx] = -1;
        newnor[idx] = dir;
      } else {
        while (idx != -1) {
          const auto cosangle = dot(nor, normalize(newnor[idx]));
          if (cosangle > SHARP_EDGE_THRESHOLD) break;
          idx = vertlist[idx];
        }
        if (idx != -1)
          newnor[idx] += dir;
        else {
          const auto old = vertlist[t[j]];
          idx = newnor.length();
          vertlist[t[j]] = newnor.length();
          vertlist.add(old);
          pm.pos.add(pm.pos[t[j]]);
          newnor.add(dir);
        }
      }
      newidx.add(idx);
    }
    newmat.add(pm.mat[i]);
  }

  // renormalize all normals
  loopv(newnor) {
    assert(any(ne(newnor[i], vec3f(zero))));
    newnor[i] = normalize(newnor[i]);
  }
  newnor.moveto(pm.nor);
  newidx.moveto(pm.idx);
  newmat.moveto(pm.mat);
}

/*-------------------------------------------------------------------------
 - build a final mesh from the qef points and quads stored in the octree
 -------------------------------------------------------------------------*/
static mesh buildmesh(octree &o, float cellsize) {
#if 0
  // build the buffers here
  vector<vec3f> posbuf, norbuf;
  vector<u32> idxbuf;
  vector<u32> mat;

  u32 num = 0, simplenum = 0, noisenum;
  loopv(ctx->m_builders) {
    const auto &b = *ctx->m_builders[i];
    const auto quadlen = b.m_quad_buffer.length();
    loopvj(b.m_quad_buffer) {
      if (b.m_quad_buffer[j].matindex == csg::MAT_SNOISE_INDEX) ++noisenum;
      else if (b.m_quad_buffer[j].matindex == csg::MAT_SIMPLE_INDEX) ++simplenum;
    }
    num += quadlen;
  }

  printf("num %d simplenum %d noisenum %d\n", num, simplenum, noisenum);
#endif
#if 1

  // build the mesh
  procmesh pm;
  buildmesh(o, pm);

  // decimate the edges and remove degenrate triangles
  decimatemesh(pm, cellsize);

  // handle sharp edges
  sharpenmesh(pm);

  // build the segment list
  vector<segment> seg;
  u32 currmat = ~0x0;
  loopv(pm.mat) {
    if (pm.mat[i] != currmat) {
      seg.add({3u*i,0u,pm.mat[i]});
      currmat = pm.mat[i];
    }
    seg.last().num += 3;
  }
#endif

#if !defined(NDEBUG)
  loopv(pm.pos) assert(!isnan(pm.pos[i].x)&&!isnan(pm.pos[i].y)&&!isnan(pm.pos[i].z));
  loopv(pm.pos) assert(!isinf(pm.pos[i].x)&&!isinf(pm.pos[i].y)&&!isinf(pm.pos[i].z));
  loopv(pm.nor) assert(!isnan(pm.nor[i].x)&&!isnan(pm.nor[i].y)&&!isnan(pm.nor[i].z));
  loopv(pm.nor) assert(!isinf(pm.nor[i].x)&&!isinf(pm.nor[i].y)&&!isinf(pm.nor[i].z));
#endif

  const auto p = pm.pos.move();
  const auto n = pm.nor.move();
  const auto idx = pm.idx.move();
  const auto s = seg.move();
  auto m = mesh(p.first, n.first, idx.first, s.first, p.second, idx.second, s.second);
#if 0
  m.m_nor = (vec3f*) MALLOC(sizeof(vec3f) * m.m_vertnum);
  loopi(m.m_vertnum) m.m_nor[i] = vec3f(zero);
  loopi(m.m_indexnum/3) {
    const auto t = &m.m_index[3*i];
    const auto edge0 = m.m_pos[t[2]]-m.m_pos[t[0]];
    const auto edge1 = m.m_pos[t[2]]-m.m_pos[t[1]];
    const auto dir = cross(edge0, edge1);
    m.m_nor[t[0]] += dir;
    m.m_nor[t[1]] += dir;
    m.m_nor[t[2]] += dir;
  }
  loopi(m.m_vertnum) m.m_nor[i] = normalize(m.m_nor[i]);
#endif
  return m;
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
  return buildmesh(o, cellsize);
}

void start() { ctx = NEWE(context); }
void finish() {
  if (ctx == NULL) return;
  loopv(ctx->m_builders) SAFE_DEL(ctx->m_builders[i]);
  DEL(ctx);
}
} /* namespace iso */
} /* namespace q */

