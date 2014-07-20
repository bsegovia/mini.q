/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso_mesh.cpp -> implements routines to compute hierchical voxel
 -------------------------------------------------------------------------*/
#include "iso.hpp"
#include "csg.hpp"
#include "qef.hpp"
#include "csg.scalar.hpp"
#include "csg.sse.hpp"
#include "csg.avx.hpp"
#include "geom.hpp"
#include "base/vector.hpp"
#include "base/task.hpp"
#include "base/console.hpp"

namespace {
STATS(iso_num);
STATS(iso_edgepos_num);
STATS(iso_edge_num);
STATS(iso_gradient_num);
STATS(iso_grid_num);
STATS(iso_octree_num);
STATS(iso_edgepos);

#if !defined(RELEASE)
static void stats() {
  printf("\n");
  printf("*************************************************\n");
  printf("** iso::voxel stats\n");
  printf("*************************************************\n");
  STATS_OUT(iso_num);
  STATS_OUT(iso_edge_num);
  STATS_RATIO(iso_edgepos_num, iso_edge_num);
  STATS_RATIO(iso_edgepos_num, iso_num);
  STATS_RATIO(iso_gradient_num, iso_num);
  STATS_RATIO(iso_grid_num, iso_num);
  STATS_RATIO(iso_octree_num, iso_num);
  printf("\n");
}
#endif /* defined(RELEASE) */
} /* namespace */

namespace q {
namespace iso {
namespace voxel {

static ref<rt::intersector> voxelbvh;
static atomic voxel_num(0);
ref<rt::intersector> get_voxel_bvh() { return voxelbvh; }

// callback to perform distance to iso-surface
static void (*isodist)(
  const csg::node *RESTRICT, const csg::array3f &RESTRICT,
  const csg::arrayf *RESTRICT, csg::arrayf &RESTRICT, csg::arrayi &RESTRICT,
  int num, const aabb &RESTRICT);

static const u32 SUBGRIDDEPTH = ilog2(SUBGRID);
static const auto DEFAULT_GRAD_STEP = 1e-3f;
static const int MAX_STEPS = 8;
static const double QEM_LEAF_MIN_ERROR = 1e-6;

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
  vec3f(0.f,0.f,0.f)/*0*/, vec3f(0.f,0.f,1.f)/*1*/,
  vec3f(0.f,1.f,1.f)/*2*/, vec3f(0.f,1.f,0.f)/*3*/,
  vec3f(1.f,0.f,0.f)/*4*/, vec3f(1.f,0.f,1.f)/*5*/,
  vec3f(1.f,1.f,1.f)/*6*/, vec3f(1.f,1.f,0.f)/*7*/
};
static const vec3i icubev[8] = {
  vec3i(0,0,0)/*0*/, vec3i(0,0,1)/*1*/,
  vec3i(0,1,1)/*2*/, vec3i(0,1,0)/*3*/,
  vec3i(1,0,0)/*4*/, vec3i(1,0,1)/*5*/,
  vec3i(1,1,1)/*6*/, vec3i(1,1,0)/*7*/
};
static const int interptable[12][2] = {
  {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},
  {6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
};
static const u32 octreechildmap[8] = {0, 4, 3, 7, 1, 5, 2, 6};
static const pair<int,int> airmat = makepair(csg::MAT_AIR_INDEX, csg::MAT_AIR_INDEX);
static const u32 FIELDDIM = SUBGRID+2;
static const u32 FIELDNUM = FIELDDIM*FIELDDIM*FIELDDIM;
static const u32 QEFNUM = SUBGRID*SUBGRID*SUBGRID;
static const u32 NOINDEX = ~0x0u;

struct edgeitem {
  vec3f org, p0, p1;
  float v0, v1;
  u32 m0, m1;
};

struct CACHE_LINE_ALIGNED edgestack {
  array<edgeitem,csg::MAXPOINTNUM> it;
  csg::array3f p, pos;
  csg::arrayf d, nd;
  csg::arrayi m;
};

INLINE pair<vec3i,int> getedge(const vec3i &start, const vec3i &end) {
  const auto lower = select(le(start,end), start, end);
  const auto delta = select(eq(start,end), vec3i(zero), vec3i(one));
  assert(reduceadd(delta) == 1);
  return makepair(lower, delta.y+2*delta.z);
}

/*-------------------------------------------------------------------------
 - voxel creation is done here
 -------------------------------------------------------------------------*/
struct builder {
  builder() :
    m_node(NULL),
    m_field(FIELDNUM),
    m_edge_index(6*FIELDNUM),
    m_stack((edgestack*)ALIGNEDMALLOC(sizeof(edgestack), CACHE_LINE_ALIGNMENT)),
    m_octree(NULL),
    m_iorg(zero),
    maxlvl(0),
    level(0)
  {}
  ~builder() {ALIGNEDFREE(m_stack);}

  struct edge {
    vec3f p, n;
    vec2i mat;
  };

  INLINE vec3f vertex(const vec3i &p) {
    const vec3i ipos = m_iorg+(p<<(int(maxlvl-level)));
    return vec3f(ipos)*cellsize;
  }
  INLINE void setoctree(const octree &o) { m_octree = &o; }
  INLINE void setorg(const vec3f &org) { m_org = org; }
  INLINE void setcellsize(float size) { cellsize = size; }
  INLINE void setnode(const csg::node *node) { m_node = node; }
  INLINE u32 field_index(const vec3i &xyz) {
    assert(all(ge(xyz,vec3i(zero))) && all(lt(xyz,vec3i(FIELDDIM))));
    return xyz.x + (xyz.y + xyz.z * FIELDDIM) * FIELDDIM;
  }
  INLINE u32 edge_index(const vec3i &start, int edge) {
    assert(all(ge(start,vec3i(zero))) && all(lt(start,vec3i(FIELDDIM))));
    const auto offset = start.x + (start.y + start.z * FIELDDIM) * FIELDDIM;
    return offset + edge * FIELDNUM;
  }
  INLINE fielditem &field(const vec3i &xyz) {return m_field[field_index(xyz)];}

  void edgepos(const csg::node *node, edgestack &stack, int num) {
    assert(num <= 64);
    auto &it = stack.it;
    auto &pos = stack.pos, &p = stack.p;
    auto &d = stack.d;
    auto &m = stack.m;

    loopk(MAX_STEPS) {
      auto box = aabb::empty();
      loopi(num) {
        const auto mid = (it[i].p0 + it[i].p1) * 0.5f;
        const auto worldmid = it[i].org+cellsize*mid;
        csg::set(p, mid, i);
        csg::set(pos, worldmid, i);
        box.pmin = min(worldmid, box.pmin);
        box.pmax = max(worldmid, box.pmax);
      }
      box.pmin -= 3.f * cellsize;
      box.pmax += 3.f * cellsize;
      isodist(m_node, pos, NULL, d, m, num, box);
      if (k != MAX_STEPS-1) {
        loopi(num) {
          assert(!isnan(d[i]));
          if (m[i] == int(it[i].m0)) {
            it[i].p0 = csg::get(p,i);
            it[i].v0 = d[i];
          } else {
            it[i].p1 = csg::get(p,i);
            it[i].v1 = d[i];
          }
        }
      } else {
        loopi(num) {
          const auto src = csg::get(p,i);
          it[i].p0 = src;
#if !defined(NDEBUG)
          assert(!isnan(src.x)&&!isnan(src.y)&&!isnan(src.z));
          assert(!isinf(src.x)&&!isinf(src.y)&&!isinf(src.z));
#endif /* !defined(NDEBUG) */
        }
      }
    }
    STATS_ADD(iso_edgepos_num, num*MAX_STEPS);
    STATS_ADD(iso_num, num*MAX_STEPS);
  }

  int delayed_voxel(const mcell &cell, const vec3i &xyz) {
    int edgemap = 0;
    loopi(12) {
      const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
      const auto m0 = cell[idx0].m, m1 = cell[idx1].m;
      if (m0 == m1) continue;
      const auto e = getedge(icubev[idx0], icubev[idx1]);
      auto &idx = m_edge_index[edge_index(xyz+e.first, e.second)];
      if (idx == NOINDEX) {
        idx = m_delayed_edges.size();
        m_delayed_edges.push_back(makepair(xyz, vec4i(idx0, idx1, m0, m1)));
      }
      edgemap |= 1<<i;
    }
    return edgemap;
  }

  void init_fields() {
    stepxyz(vec3i(zero), vec3i(FIELDDIM), vec3i(4)) {
      auto &pos = m_stack->p;
      auto &d = m_stack->d;
      auto &m = m_stack->m;
      const auto p = vertex(sxyz);
      const auto box = aabb(p-2.f*cellsize, p+6.f*cellsize);
      int index = 0;
      const auto end = min(sxyz+4,vec3i(FIELDDIM));
      loopxyz(sxyz, end) csg::set(pos, vertex(xyz), index++);
      isodist(m_node, pos, NULL, d, m, index, box);
#if !defined(NDEBUG)
      loopi(index) assert(d[i] <= 0.f || m[i] == csg::MAT_AIR_INDEX);
      loopi(index) assert(d[i] >= 0.f || m[i] != csg::MAT_AIR_INDEX);
#endif /* NDEBUG */
      STATS_ADD(iso_num, index);
      STATS_ADD(iso_grid_num, index);
      index = 0;
      loopxyz(sxyz, end) {
        field(xyz) = fielditem(d[index], m[index]);
        ++index;
      }
    }
  }

  void init_edges() {
    m_edge_index.memset(u8(0xff));
    m_edges.resize(0);
    m_delayed_edges.resize(0);
  }

  void init_voxels() {
    m_delayed_voxels.resize(0);
    loopxyz(zero, SUBGRID) {
      if (abs(field(xyz).d) > 2.f*cellsize) continue;

      // figure out if the voxel lies on an boundary
      mcell cell;
      loopk(8) cell[k] = field(xyz+icubev[k]);
      const auto res = delayed_voxel(cell,xyz);
      if (res == 0 || res == 0xff) continue;

      // if so, we will need to process it. pass 'finish_edges' will compute the
      // intersection point on the edge and their normals
      m_delayed_voxels.push_back(makepair(xyz, res));
    }
  }

  void finish_edges() {
    const auto len = m_delayed_edges.size();
    STATS_ADD(iso_edge_num, len);
    for (int i = 0; i < len; i += 64) {
      auto &it = m_stack->it;

      // step 1 - run bisection with packets of (up-to) 64 points. we need
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
        auto &p = m_stack->p;
        auto &d = m_stack->d;
        auto &m = m_stack->m;
        auto &nd = m_stack->nd;
        auto box = aabb::empty();
        loopk(subnum) {
          const auto center = it[j+k].org + it[j+k].p0 * cellsize;
          csg::set(p, center,    4*k+0);
          csg::set(p, center-dx, 4*k+1);
          csg::set(p, center-dy, 4*k+2);
          csg::set(p, center-dz, 4*k+3);
          box.pmin = min(center, box.pmin);
          box.pmax = max(center, box.pmax);
        }
        box.pmin -= 3.f * cellsize;
        box.pmax += 3.f * cellsize;

        loopk(4*subnum) {
          const auto m0 = it[j+k/4].m0;
          const auto m1 = it[j+k/4].m1;
          bool const solidsolid = m0 != csg::MAT_AIR_INDEX && m1 != csg::MAT_AIR_INDEX;
          nd[k] = solidsolid ? cellsize : 0.f;
        }
        isodist(m_node, p, &nd, d, m, 4*subnum, box);
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
          m_edges.push_back({p,n,vec2i(it[j+k].m0,it[j+k].m1)});
        }
      }
    }
  }

  void finish_voxels(octree::node &node) {
    bvh = NULL;
    vector<rt::primitive> voxels;
    loopv(m_delayed_voxels) {
      const auto &item = m_delayed_voxels[i];
      const auto xyz = item.first;
      const auto edgemap = item.second;

      // if this vertex does not belong this leaf, skip it. we test all signs
      if ((xyz.x|xyz.y|xyz.z)>>31) continue;

      // get the edge from the list of edges we initialized in
      // previous pass 'init_edges'
      array<vec3f,12> p, n;
      int num = 0;
      loopi(12) {
        if ((edgemap & (1<<i)) == 0) continue;

        // fetch the edge
        const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
        const auto e = getedge(icubev[idx0], icubev[idx1]);
        const auto idx = m_edge_index[edge_index(xyz+e.first, e.second)];
        assert(idx != NOINDEX);

        // we do not care about multi-material edges for voxel rendering
        if (m_edges[idx].mat.x != csg::MAT_AIR_INDEX &&
            m_edges[idx].mat.y != csg::MAT_AIR_INDEX)
          continue;
        p[num] = vertex(xyz)+cellsize*(m_edges[idx].p+vec3f(e.first));
        n[num] = m_edges[idx].n;
        ++num;
      }
      if (num == 0) continue;

      // now figure out if the voxels contains some discontinuities
      bool discontinuous = false;
      rangei(1,num) if (dot(n[i],n[0]) < 0.9f) {
        discontinuous = true;
        break;
      }

      // since we use only one plane, try to find the best one i.e. the one
      // which is as different as possible as the principal axis
      int best = -1;
      float best_score = FLT_MAX;
      loopi(num) {
        const auto scorex = abs(dot(n[i], vec3f(1.f,0.f,0.f)));
        const auto scorey = abs(dot(n[i], vec3f(0.f,1.f,0.f)));
        const auto scorez = abs(dot(n[i], vec3f(0.f,0.f,1.f)));
        const auto score = max(scorex, scorey, scorez);
        if (score < best_score) {
          best = i;
          best_score = score;
        }
      }

      // for now, we just encode one plane. TODO encode more planes
      const auto pmin = vertex(xyz);
      const auto pmax = pmin + vec3f(cellsize);
      const aabb box(pmin, pmax);
      const auto d0 = -dot(p[best], n[best]);
      const auto d1 = d0 + cellsize;
      voxels.push_back(rt::primitive(box, n[best], d0, d1, discontinuous));
    }

    if (voxels.size() > 0) {
      bvh = NEW(rt::intersector, &voxels[0], voxels.size());
      voxel_num += voxels.size();
    }
  }

  void build(octree::node &node) {
    init_fields();
    init_edges();
    init_voxels();
    finish_edges();
    finish_voxels(node);
  }

  const csg::node *m_node;
  ref<rt::intersector> bvh;
  vector<fielditem> m_field;
  vector<u32> m_edge_index;
  vector<edge> m_edges;
  vector<pair<vec3i,vec4i>> m_delayed_edges;
  vector<pair<vec3i,int>> m_delayed_voxels;
  edgestack *m_stack;
  const octree *m_octree;
  vec3f m_org;
  vec3i m_iorg;
  float cellsize;
  u32 m_qefnum;
  u32 maxlvl, level;
};

/*-------------------------------------------------------------------------
 - multi-threaded implementation of the (hierarchical) voxel creation
 -------------------------------------------------------------------------*/
static THREAD builder *local_builder = NULL;
struct context {
  INLINE context() : m_mutex(SDL_CreateMutex()) {}
  SDL_mutex *m_mutex;
  vector<builder*> m_builders;
};
static context *ctx = NULL;

// what to run per task iteration
struct workitem {
  const csg::node *csgnode;
  struct octree::node *octnode;
  struct octree *oct;
  vec3i iorg;
  vec3f org;
  int level;
  int maxlvl;
  float cellsize;
  ref<rt::intersector> bvh;
};

// build the bvh of voxel bvh
struct task_build_voxel_bvh : public task {

  INLINE task_build_voxel_bvh(vector<workitem> &items) :
    task("task_build_voxel_bvh"), items(items)
  {}
  virtual void run(u32) {
    vector<rt::primitive> subbvh;
    loopv(items) if (items[i].bvh) subbvh.push_back(rt::primitive(items[i].bvh));
    con::out("voxel bvh: %d sub-bvhs", subbvh.size());
    con::out("voxel num: %d", int(voxel_num));
    con::out("average voxel per sub-bvh: %f", float(int(voxel_num))/ float(subbvh.size()));
    if (subbvh.size() > 0)
      voxelbvh = NEW(rt::intersector, &subbvh[0], subbvh.size());
  }
  vector<workitem> &items;
};

// create the small sub-bvh of voxels
struct task_build_voxel_subbvh : public task {
  INLINE task_build_voxel_subbvh(vector<workitem> &items) :
    task("task_build_voxel_subbvh", items.size()), items(items)
  {}

  virtual void run(u32 idx) {
    if (local_builder == NULL) {
      local_builder = NEWE(builder);
      SDL_LockMutex(ctx->m_mutex);
      ctx->m_builders.push_back(local_builder);
      SDL_UnlockMutex(ctx->m_mutex);
    }
    const auto &job = items[idx];
    local_builder->m_octree = job.oct;
    local_builder->m_iorg = job.iorg;
    local_builder->level = job.octnode->level;
    local_builder->maxlvl = job.maxlvl;
    local_builder->setcellsize(job.cellsize);
    local_builder->setnode(job.csgnode);
    local_builder->setorg(job.org);
    local_builder->build(*job.octnode);
    items[idx].bvh = local_builder->bvh;
  }
  vector<workitem> &items;
};

// create the bvh of voxels
struct task_build_voxel : public task {
  INLINE task_build_voxel(octree &o, const csg::node &csgnode,
                          const vec3f &org, float cellsize,
                          u32 dim) :
    task("task_build_voxel", 1, 1),
    oct(&o), csgnode(&csgnode),
    org(org), cellsize(cellsize), dim(dim)
  {
    assert(ispoweroftwo(dim) && dim % SUBGRID == 0);
    maxlvl = ilog2(dim / SUBGRID);
  }

  virtual void run(u32) {
    build(oct->m_root);
    build_iso_jobs(oct->m_root);
    ref<task> subbvhtask = NEW(task_build_voxel_subbvh, items);
    ref<task> bvhtask = NEW(task_build_voxel_bvh, items);
    subbvhtask->starts(*bvhtask);
    bvhtask->ends(*this);
    subbvhtask->scheduled();
    bvhtask->scheduled();
  }

  INLINE vec3f pos(const vec3i &xyz) {return org+cellsize*vec3f(xyz);}

  void build(octree::node &node, const vec3i &xyz = vec3i(zero), u32 level = 0) {
    node.level = level;
    node.org = xyz;

    // bounding box of this octree cell
    const auto lod = maxlvl - level;
    const vec3f pmin = pos(xyz - int(4<<lod));
    const vec3f pmax = pos(xyz + int((SUBGRID+4)<<lod));

    // center of the box where to evaluate the distance field
    const auto cellnum = int(dim >> level);
    const auto icenter = xyz + cellnum/2;
    const auto center = pos(icenter);
    const auto dist = csg::dist(csgnode, center, aabb(pmin,pmax));
    STATS_INC(iso_octree_num);
    STATS_INC(iso_num);
    if (abs(dist) > sqrt(3.f) * cellsize * float(cellnum/2+2)) {
      node.isleaf = node.empty = 1;
      return;
    }
    if (cellnum == SUBGRID) {
#if DEBUGOCTREE
      const vec3f minpos = pos(xyz) - vec3f(debugsize);
      const vec3f maxpos = pos(xyz+vec3i(SUBGRID)) + vec3f(debugsize);
      if (any(lt(debugpos,minpos)) || any(gt(debugpos,maxpos))) {
        node.empty = node.isleaf = 1;
        return;
      }
#endif /* DEBUGOCTREE */
      node.leaf = NEWE(octree::leaftype);
      node.isleaf = 1;
    } else {
      node.children = NEWAE(octree::node, 8);
      loopi(8) {
        const auto childxyz = xyz+cellnum*icubev[i]/2;
        build(node.children[i], childxyz, level+1);
      }
    }
  }

  void build_iso_jobs(octree::node &node, const vec3i &xyz = vec3i(zero)) {
    if (node.isleaf && !node.empty) {
      workitem job;
      job.oct = oct;
      job.octnode = &node;
      job.csgnode = csgnode;
      job.iorg = xyz;
      job.maxlvl = maxlvl;
      job.level = node.level;
      job.cellsize = float(1<<(maxlvl-node.level)) * cellsize;
      job.org = pos(xyz);
      items.push_back(job);
    } else if (!node.isleaf) loopi(8) {
      const auto cellnum = dim >> node.level;
      const auto childxyz = xyz+int(cellnum/2)*icubev[i];
      build_iso_jobs(node.children[i], childxyz);
    }
  }

  vector<workitem> items;
  octree *oct;
  const csg::node *csgnode;
  vec3f org;
  float cellsize;
  u32 dim, maxlvl;
};

ref<task> create_voxel_task(octree &o, const csg::node &node, const vec3f &org, u32 cellnum, float cellsize) {
  return NEW(task_build_voxel, o, node, org, cellsize, cellnum);
}

void start() {
  using namespace sys;
  if (/* hasfeature(CPU_YMM) && */ sys::hasfeature(CPU_AVX)) {
    con::out("iso::voxel: avx path selected");
    isodist = csg::avx::dist;
  } else if (hasfeature(CPU_SSE) && hasfeature(CPU_SSE2)) {
    con::out("iso::voxel: sse path selected");
    isodist = csg::sse::dist;
  } else {
    con::out("iso::voxel: warning: slow path for isosurface extraction");
    isodist = csg::dist;
  }
  ctx = NEWE(context);
}

void finish() {
  if (ctx == NULL) return;
#if !defined(RELEASE)
  stats();
#endif
  loopv(ctx->m_builders) SAFE_DEL(ctx->m_builders[i]);
  DEL(ctx);
}
} /* namespace voxel */
} /* namespace iso */
} /* namespace q */

