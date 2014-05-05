/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso.cpp -> implements routines for iso surface
 -------------------------------------------------------------------------*/
#include "iso.hpp"
#include "csg.hpp"
#include "qef.hpp"
#include "csgscalar.hpp"
#include "csgsse.hpp"
#include "csgavx.hpp"
#include "geom.hpp"
#include "base/vector.hpp"
#include "base/task.hpp"
#include "base/console.hpp"

STATS(iso_num);
STATS(iso_edgepos_num);
STATS(iso_edge_num);
STATS(iso_gradient_num);
STATS(iso_grid_num);
STATS(iso_octree_num);
STATS(iso_qef_num);
STATS(iso_edgepos);

#if !defined(RELEASE)
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
#endif /* defined(RELEASE) */

#define DEBUGOCTREE 0
#if DEBUGOCTREE
static const q::vec3f debugpos(19.f,3.0f,13.f);
static const float debugsize = 0.8f;
#endif /* DEBUGOCTREE */

//#define CSGVER csg::avx
//#define CSGVER csg::sse
#define CSGVER csg

namespace q {
namespace iso {

static const u32 SUBGRIDDEPTH = ilog2(SUBGRID);
static const auto DEFAULT_GRAD_STEP = 1e-3f;
static const int MAX_STEPS = 8;
static const double QEM_LEAF_MIN_ERROR = 1e-6;

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

/*-------------------------------------------------------------------------
 - leafoctree implementation
 -------------------------------------------------------------------------*/
void leafoctreebase::init() {
  root.setsize(1);
  root[0].setemptyleaf();
}

void leafoctreebase::insert(vec3i xyz, int ptidx) {
  assert(all(ge(xyz,vec3i(zero))) && "out-of-bound vertex");
  assert(all(lt(xyz,vec3i(SUBGRID))) && "out-of-bound vertex");
  u32 level = 0, idx = 0;
  for (;;) {
    if (level == SUBGRIDDEPTH) {
      root[idx].idx = ptidx;
      root[idx].isleaf = 1;
      root[idx].empty = 0;
      break;
    }
    if (root[idx].idx == 0) {
      const auto childidx = root.length();
      root[idx].isleaf = root[idx].empty = 0;
      root[idx].idx = childidx;
      root.setsize(childidx+8);
      loopi(8) root[childidx+i].setemptyleaf();
    }
    idx = descend(xyz, level, idx);
    ++level;
  }
}

int leafoctreebase::getidx(vec3i xyz) {
  assert(all(ge(xyz,vec3i(zero))) && "out-of-bound vertex");
  assert(all(lt(xyz,vec3i(SUBGRID))) && "out-of-bound vertex");
  u32 level = 0, idx = 0;
  for (;;) {
    const auto node = &root[idx];
    if (node->empty) return -1;
    if (node->isleaf) return node->idx;
    idx = descend(xyz, level, idx);
    ++level;
  }
}

INLINE u32 leafoctreebase::descend(vec3i &xyz, u32 level, u32 idx) {
  const auto logsize = vec3i(SUBGRIDDEPTH-level-1);
  const auto bits = xyz >> logsize;
  const auto child = octreechildmap[bits.x | (bits.y<<1) | (bits.z<<2)];
  assert(all(ge(xyz, icubev[child] << logsize)));
  xyz -= icubev[child] << logsize;
  idx = root[idx].idx+child;
  return idx;
}

/*-------------------------------------------------------------------------
 - global octree implementation
 -------------------------------------------------------------------------*/
octree::node::~node() {
  if (isleaf)
    SAFE_DEL(leaf);
  else
    SAFE_DELA(children);
}

const octree::node *octree::findleaf(vec3i xyz) const {
  if (any(lt(xyz,vec3i(zero))) || any(ge(xyz,vec3i(m_dim)))) return NULL;
  int level = 0;
  const node *node = &m_root;
  for (;;) {
    if (node->isleaf) return node;
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
 - temporary structure to handle *leaf* mesh data before merging similar
 - vertices using qem
 -------------------------------------------------------------------------*/
struct procleaf {
  struct vertex {
    INLINE bool multimat() const {return mat==airmat;}
    vec3f world;       // position in world space
    vec3f local;       // relative position in local grid
    qef::qem qem;      // qem matrix used to merge vertices
    vec3<char> xyz;    // coordinates in the local grid
    pair<int,int> mat; // pair of material used to build the qef
  };

  // merge similar qef points together
  void merge(int idx = 0);

  // removed degenerated quads
  void decimate();

  leafoctree<vertex> leaf;
};

void procleaf::merge(int idx) {
  const auto node = leaf.getnode(idx);
  if (node->isleaf) return;
  assert(!node->empty && "node cannot be empty here");

  int num = 0;
  auto mat = airmat;

  // first merge all children
  loopi(8) {
    const auto childidx = node->idx+i;
    const auto child = leaf.getnode(childidx);
    if (child->empty) continue;
    merge(childidx);
  }

  // gather all points from the children nodes
  array<int,8> children;
  loopi(8) {
    const auto childidx = node->idx+i;
    const auto child = leaf.getnode(childidx);
    if (child->empty) continue;
    if (!child->isleaf) return;
    const auto qef = &leaf.pts[child->idx];
    if (qef->multimat())
      return;
    if (mat == airmat)
      mat = qef->mat;
    else if (mat != qef->mat)
      return;
    children[num++] = child->idx;
  }

  // one point means nothing to merge
  assert(num != 0 && "empty leaf marked as non-empty");
  if (num == 1) {
    node->isleaf = 1;
    node->idx = children[0];
    return;
  }

  // now, try to merge the children using qem
  auto q = leaf.pts[children[0]].qem;
  rangei(1,num) q += leaf.pts[children[i]].qem;

  // evaluate the best candidate among all of them
  float bestcost = FLT_MAX;
  int best = -1;
  loopi(num) {
    const auto idx = children[i];
    const auto cost = q.error(leaf.pts[idx].local,QEM_LEAF_MIN_ERROR);
    if (bestcost > cost) {
      bestcost = cost;
      best = idx;
    }
  }

  assert(best != -1 && "unable to find candidate");
  if (bestcost > QEM_LEAF_MIN_ERROR) return;
  node->isleaf = 1;
  node->idx = best;
}

/*-------------------------------------------------------------------------
 - iso surface extraction is done here
 -------------------------------------------------------------------------*/
struct gridbuilder {
  gridbuilder() :
    m_node(NULL),
    m_field(FIELDNUM),
    m_qef_index(QEFNUM),
    m_edge_index(6*FIELDNUM),
    stack((edgestack*)ALIGNEDMALLOC(sizeof(edgestack), CACHE_LINE_ALIGNMENT)),
    m_octree(NULL),
    m_iorg(zero),
    maxlvl(0),
    level(0)
  {}
  ~gridbuilder() { ALIGNEDFREE(stack); }

  struct edge {
    vec3f p, n;
    vec2i mat;
  };

  struct qef_output {
    INLINE qef_output(vec3f p, vec3f n, bool valid):p(p),n(n),valid(valid){}
    vec3f p, n;
    bool valid;
  };

  INLINE vec3f vertex(const vec3i &p) {
    const vec3i ipos = m_iorg+(p<<(int(maxlvl-level)));
    return vec3f(ipos)*cellsize;
  }
  INLINE void setoctree(const octree &o) { m_octree = &o; }
  INLINE void setorg(const vec3f &org) { m_org = org; }
  INLINE void setcellsize(float size) { cellsize = size; }
  INLINE void setnode(const csg::node *node) { m_node = node; }
  INLINE u32 qef_index(const vec3i &xyz) const {
    assert(all(ge(xyz,vec3i(zero))) && all(lt(xyz,vec3i(SUBGRID))));
    return xyz.x + (xyz.y + xyz.z * SUBGRID) * SUBGRID;
  }
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

  void initfield() {
    stepxyz(vec3i(zero), vec3i(FIELDDIM), vec3i(4)) {
      auto &pos = stack->p;
      auto &d = stack->d;
      auto &m = stack->m;
      const auto p = vertex(sxyz);
      const auto box = aabb(p-2.f*cellsize, p+6.f*cellsize);
      int index = 0;
      const auto end = min(sxyz+4,vec3i(FIELDDIM));
      loopxyz(sxyz, end) csg::set(pos, vertex(xyz), index++);
      CSGVER::dist(m_node, pos, NULL, d, m, index, box);
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

  void initedge() {
    m_edge_index.memset(0xff);
    m_edges.setsize(0);
    delayed_edges.setsize(0);
  }

  void initqef() {
    m_qefnum = 0;
    m_qef_index.memset(0xff);
    delayed_qef.setsize(0);
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
        idx = delayed_edges.length();
        delayed_edges.add(makepair(xyz, vec4i(idx0, idx1, m0, m1)));
      }
      edgemap |= 1<<i;
    }
    return edgemap;
  }

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
      CSGVER::dist(m_node, pos, NULL, d, m, num, box);
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
#endif /* NDEBUG */
        }
      }
    }
    STATS_ADD(iso_edgepos_num, num*MAX_STEPS);
    STATS_ADD(iso_num, num*MAX_STEPS);
  }

  void finishedges() {
    const auto len = delayed_edges.length();
    STATS_ADD(iso_edge_num, len);
    for (int i = 0; i < len; i += 64) {
      auto &it = stack->it;

      // step 1 - run bisection with packets of (up-to) 64 points. we need
      // to be careful FP wise. We ensure here that the position computation is
      // invariant from grids to grids such that neighbor grids will output the
      // exact same result
      const int num = min(64, len-i);
      loopj(num) {
        const auto &e = delayed_edges[i+j];
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
      edgepos(m_node, *stack, num);

      // step 2 - compute normals for each point using packets of 16x4 points
      const auto dx = vec3f(DEFAULT_GRAD_STEP, 0.f, 0.f);
      const auto dy = vec3f(0.f, DEFAULT_GRAD_STEP, 0.f);
      const auto dz = vec3f(0.f, 0.f, DEFAULT_GRAD_STEP);
      for (int j = 0; j < num; j += 16) {
        const int subnum = min(num-j, 16);
        auto &p = stack->p;
        auto &d = stack->d;
        auto &m = stack->m;
        auto &nd = stack->nd;
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
        CSGVER::dist(m_node, p, &nd, d, m, 4*subnum, box);
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
          m_edges.add({p,n,vec2i(it[j+k].m0,it[j+k].m1)});
        }
      }
    }
  }

  void finishvertices() {
    loopv(delayed_qef) {
      const auto &item = delayed_qef[i];
      const auto xyz = item.first;
      const auto edgemap = item.second;

      // this vertex does not belong this leaf. skip it. we test all three sign
      if ((xyz.x|xyz.y|xyz.z)>>31) continue;

      // get the intersection points between the surface and the cube edges
      qef::qem q;
      vec3f p[12], n[12], mass(zero);
      vec3f nor = zero;
      pair<int,int> mat = airmat;
      int num = 0, multimat = false;
      const auto gridpos = vec3f(xyz) * cellsize;
      loopi(12) {
        if ((edgemap & (1<<i)) == 0) continue;
        const auto idx0 = interptable[i][0], idx1 = interptable[i][1];
        const auto e = getedge(icubev[idx0], icubev[idx1]);
        const auto idx = m_edge_index[edge_index(xyz+e.first, e.second)];
        assert(idx != NOINDEX);
        const auto mat0 = min(m_edges[idx].mat.x, m_edges[idx].mat.y);
        const auto mat1 = max(m_edges[idx].mat.x, m_edges[idx].mat.y);
        const auto edgemat = makepair(mat0, mat1);
        if (mat == airmat)
          mat = edgemat;
        else if (edgemat != mat)
          multimat = true;

        p[num] = m_edges[idx].p+vec3f(e.first);
        n[num] = m_edges[idx].n;
        q += qef::qem(n[num], gridpos+cellsize*p[num]);
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
      const auto worldpos = vertex(xyz) + pos*cellsize;
      const auto localpos = (vec3f(xyz)+pos)*cellsize;

      // insert the point in the leaf octree
      pl.leaf.insert(xyz,pl.leaf.pts.length());
      pl.leaf.pts.add({worldpos,localpos,q,xyz,multimat?airmat:mat});
    }
  }

  void tesselate() {
    loopxyz(vec3i(zero), vec3i(FIELDDIM)) {
      const auto startfield = field(xyz);
      const auto startsign = startfield.d < 0.f ? 1 : 0;
      if (abs(field(xyz).d) > 2.f*cellsize) continue;

      // some quads belong to our neighbor. we will not push them but we need to
      // compute their vertices such that our neighbor can output these quads
      const auto outside = any(eq(xyz,0));

      // look at the three edges that start on xyz
      loopi(3) {

        // is it an actual edge?
        const auto end = xyz+axis[i];
        if (any(ge(end,vec3i(FIELDDIM)))) continue;
        const auto endfield = field(end);
        const int endsign = endfield.d < 0.f ? 1 : 0;
        if (startsign == endsign) continue;

        // we found one edge. we output one quad for it
        const auto axis0 = axis[(i+1)%3];
        const auto axis1 = axis[(i+2)%3];
        const vec3i p[] = {xyz, xyz-axis0, xyz-axis0-axis1, xyz-axis1};
        loopj(4) {
          const auto np = p[j];
          if (any(lt(np,vec3i(zero))) || any(ge(np,vec3i(SUBGRID))))
            continue;
          const auto idx = qef_index(np);
          if (m_qef_index[idx] == NOINDEX) {
            mcell cell;
            loopk(8) cell[k] = field(np+icubev[k]);
            m_qef_index[idx] = m_qefnum++;
            delayed_qef.add(makepair(np, delayed_qef_vertex(cell, np)));
            STATS_INC(iso_qef_num);
          }
        }

        // we must use a more compact storage for it
        if (outside) continue;
        const auto qor = startsign==1 ? quadorder : quadorder_cc;
        const quad q = {{p[qor[0]],p[qor[1]],p[qor[2]],p[qor[3]]},
          max(startfield.m, endfield.m)
        };
        pl.leaf.quads.add(q);
      }
    }
  }

  void outputoctree(octree::node &node, int src = 0, int dst = 0) {
    const auto from = pl.leaf.getnode(src);
    const auto to = node.leaf->getnode(dst);
    to->isleaf = from->isleaf;
    to->empty = from->empty;

    // if this is a leaf we stop here
    if (from->isleaf) {
      if (!from->empty) {
        to->idx = node.leaf->pts.length();
        node.leaf->pts.add({pl.leaf.pts[from->idx].world,-1});
      }
      return;
    }

    // otherwise, we create all 8 children and recurse
    const auto idx = node.leaf->root.length();
    to->idx = idx;
    node.leaf->root.setsize(idx+8);
    loopi(8) outputoctree(node, from->idx+i, idx+i);
  }

  void output(octree::node &node) {
    node.leaf->root.setsize(1);
    outputoctree(node);
    node.leaf->root.refit();
    node.leaf->pts.refit();
    pl.leaf.quads.moveto(node.leaf->quads);
  }

  void build(octree::node &node) {
    pl.leaf.init();
    initfield();
    initedge();
    initqef();
    tesselate();
    finishedges();
    finishvertices();
    pl.merge();
    output(node);
  }

  const csg::node *m_node;
  vector<fielditem> m_field;
  vector<u32> m_qef_index;
  vector<u32> m_edge_index;
  vector<edge> m_edges;
  vector<pair<vec3i,vec4i>> delayed_edges;
  vector<pair<vec3i,int>> delayed_qef;
  edgestack *stack;
  const octree *m_octree;
  procleaf pl;
  vec3f m_org;
  vec3i m_iorg;
  float cellsize;
  u32 m_qefnum;
  u32 maxlvl, level;
};

/*-------------------------------------------------------------------------
 - multi-threaded implementation of the iso surface extraction
 -------------------------------------------------------------------------*/
static THREAD gridbuilder *localbuilder = NULL;
struct context {
  INLINE context() : m_mutex(SDL_CreateMutex()) {}
  SDL_mutex *m_mutex;
  vector<gridbuilder*> m_builders;
};
static context *ctx = NULL;

// run the contouring part per leaf of octree using small grids
struct contouringtask : public task {

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
  };

  INLINE contouringtask(vector<workitem> &items) :
    task("contouringtask", items.length()), items(items)
  {}

  virtual void run(u32 idx) {
    if (localbuilder == NULL) {
      localbuilder = NEWE(gridbuilder);
      SDL_LockMutex(ctx->m_mutex);
      ctx->m_builders.add(localbuilder);
      SDL_UnlockMutex(ctx->m_mutex);
    }
    const auto &job = items[idx];
    localbuilder->m_octree = job.oct;
    localbuilder->m_iorg = job.iorg;
    localbuilder->level = job.octnode->level;
    localbuilder->maxlvl = job.maxlvl;
    localbuilder->setcellsize(job.cellsize);
    localbuilder->setnode(job.csgnode);
    localbuilder->setorg(job.org);
    localbuilder->build(*job.octnode);
  }
  vector<workitem> &items;
};

// build the octree topology needed to run contouring
struct isotask : public task {
  typedef contouringtask::workitem workitem;
  INLINE isotask(octree &o, const csg::node &csgnode,
                 const vec3f &org, float cellsize,
                 u32 dim) :
    task("isotask", 1),
    oct(&o), csgnode(&csgnode),
    org(org), cellsize(cellsize),
    dim(dim)
  {
    assert(ispoweroftwo(dim) && dim % SUBGRID == 0);
    maxlvl = ilog2(dim / SUBGRID);
  }

  virtual void run(u32) {
    build(oct->m_root);
    preparejobs(oct->m_root);
    spawnnext();
  }

  INLINE vec3f pos(const vec3i &xyz) {
    return org+cellsize*vec3f(xyz);
  }

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

  void preparejobs(octree::node &node, const vec3i &xyz = vec3i(zero)) {
    if (node.isleaf && !node.empty) {
      auto &job = items.add();
      job.oct = oct;
      job.octnode = &node;
      job.csgnode = csgnode;
      job.iorg = xyz;
      job.maxlvl = maxlvl;
      job.level = node.level;
      job.cellsize = float(1<<(maxlvl-node.level)) * cellsize;
      job.org = pos(xyz);
    } else if (!node.isleaf) loopi(8) {
      const auto cellnum = dim >> node.level;
      const auto childxyz = xyz+int(cellnum/2)*icubev[i];
      preparejobs(node.children[i], childxyz);
    }
  }

  void spawnnext() {
    ref<task> contouring = NEW(contouringtask, items);
    contouring->ends(*this);
    contouring->scheduled();
  }

  vector<workitem> items;
  octree *oct;
  const csg::node *csgnode;
  vec3f org;
  float cellsize;
  u32 dim, maxlvl;
};

geom::mesh dc(const vec3f &org, u32 cellnum, float cellsize, const csg::node &csgnode) {
  octree o(cellnum);
  geom::mesh m;

  ref<task> meshtask = geom::buildmesh(m, o, cellsize);
  ref<task> contouringtask = NEW(isotask, o, csgnode, org, cellsize, cellnum);
  contouringtask->starts(*meshtask);
  meshtask->scheduled();
  contouringtask->scheduled();
  meshtask->wait();

#if !defined(RELEASE)
  stats();
#endif /* defined(RELEASE) */

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

