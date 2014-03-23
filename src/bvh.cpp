/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvh.cpp -> implements bvh routines (build and traversal)
 -------------------------------------------------------------------------*/
#include <cstring>
#include <cfloat>
#include <cmath>
#include "bvh.hpp"
#include "console.hpp"
#include "script.hpp"
#include "math.hpp"
#include "stl.hpp"
#include "sys.hpp"

namespace q {
namespace bvh {

// build options
VAR(maxprimitivenum, 1, 8, 16);
VAR(sahintersectioncost, 1, 4, 16);
VAR(sahtraversalcost, 1, 4, 16);
VAR(bvhstatitics, 0, 1, 1);

struct waldtriangle {
  vec2f bn, cn, vertk, n;
  float nd;
  u32 k:2, sign:1, num:29;
  u32 id, matid;
};

struct intersector {
  static const u32 NONLEAF = 0x0;
  static const u32 TRILEAF = 0x2;
  static const u32 ISECLEAF = 0x3;
  static const u32 MASK = 0x3;
  static const u32 SHIFT = 2;
  struct node {
    aabb box;
    union {
      struct {
        u32 offsetflag;
        u32 axis;
      };
      uintptr prim;
    };
    template <typename T>
    INLINE const T *getptr(void) const {return (const T*)(prim&~uintptr(MASK));}
    template <typename T>
    INLINE void setptr(const T *ptr) { prim = (prim&uintptr(MASK))|uintptr(ptr); }
    INLINE u32 getoffset(void) const { return offsetflag>>SHIFT; }
    INLINE u32 getaxis(void) const { return axis; }
    INLINE u32 getflag(void) const { return offsetflag & MASK; }
    INLINE u32 isleaf(void) const { return getflag(); }
    INLINE void setoffset(u32 offset) { offsetflag = (offset<<SHIFT)|getflag(); }
    INLINE void setaxis(u32 d) { axis = d; }
    INLINE void setflag(u32 flag) { offsetflag = (offsetflag&~MASK)|flag; }
  };
  node *root;
  vector<waldtriangle> acc;
};
static_assert(sizeof(intersector::node) == 32,"invalid node size");

aabb getaabb(const struct intersector *isec) {
  return isec->root[0].box;
}

struct centroid {
  INLINE centroid(void) {}
  INLINE centroid(const primitive &h) {
    if (h.type == primitive::TRI)
      v = 1.f/3.f*(h.v[0]+h.v[1]+h.v[2]);
    else
      v = (h.v[0]+h.v[1])/2.f;
  }
  vec3f v;
};

enum {OTHERAXISNUM = 2};
enum {ONLEFT, ONRIGHT};

// n log(n) compiler with bounding box sweeping and SAH heuristics
struct compiler {
  compiler(void) : n(0), accnum(0), currid(0), leafnum(0), nodenum(0) {}
  void injection(const primitive *soup, u32 primnum);
  void compile(void);
  vector<u8> istri;
  vector<s32> pos;
  vector<u32> ids[3];
  vector<u32> tmpids;
  vector<aabb> boxes;
  vector<aabb> rlboxes;
  const primitive *prims;
  vector<waldtriangle> acc;
  intersector::node *root;
  s32 n, accnum;
  u32 currid;
  aabb scenebox;
  u32 leafnum, nodenum;
};

template<u32 axis> struct sorter {
  const vector<centroid> &centroids;
  sorter(const vector<centroid> &c) : centroids(c) {}
  INLINE int operator() (const u32 a, const u32 b) const  {
    return centroids[a].v[axis] < centroids[b].v[axis];
  }
};

void compiler::injection(const primitive *soup, const u32 primnum) {
  vector<centroid> centroids;

  root = NEWAE(intersector::node,2*primnum+1);

  loopi(3) ids[i].setsize(primnum);
  pos.setsize(primnum);
  tmpids.setsize(primnum);
  centroids.setsize(primnum);
  boxes.setsize(primnum);
  rlboxes.setsize(primnum);
  istri.setsize(primnum);
  n = primnum;

  scenebox = aabb(FLT_MAX, -FLT_MAX);
  loopi(n) {
    istri[i] = soup[i].type == primitive::TRI;
    centroids[i] = centroid(soup[i]);
    boxes[i] = soup[i].getaabb();
    scenebox.compose(boxes[i]);
  }

  loopi(3) loopj(n) ids[i][j] = j;
  quicksort(&ids[0][0], &ids[0][0]+primnum, sorter<0>(centroids));
  quicksort(&ids[1][0], &ids[1][0]+primnum, sorter<1>(centroids));
  quicksort(&ids[2][0], &ids[2][0]+primnum, sorter<2>(centroids));
  prims = soup;
  acc.setsize(primnum);
}

struct partition {
  aabb boxes[2];
  float cost;
  u32 axis;
  int first[2], last[2];
  partition() {}
  partition(int f, int l, u32 d) : cost(FLT_MAX), axis(d) {
    boxes[ONLEFT] = boxes[ONRIGHT] = aabb(FLT_MAX, -FLT_MAX);
    first[ONRIGHT] = first[ONLEFT] = f;
    last[ONRIGHT] = last[ONLEFT] = l;
  }
};

// sweep the bounding boxen from left to right
template <u32 axis> INLINE partition sweep(compiler &c, int first, int last) {
  partition part(first, last, axis);

  // compute the inclusion sequence
  c.rlboxes[c.ids[axis][last]] = c.boxes[c.ids[axis][last]];
  for (s32 j = last - 1; j >= first; --j) {
    c.rlboxes[c.ids[axis][j]] = c.boxes[c.ids[axis][j]];
    c.rlboxes[c.ids[axis][j]].compose(c.rlboxes[c.ids[axis][j + 1]]);
  }

  // sweep from left to right and find the best partition
  aabb box(FLT_MAX, -FLT_MAX);
  s32 primnum = (last-first)+1, n = 1;
  part.cost = FLT_MAX;
  bool alltris = true;
  for (s32 j = first; j < last; ++j) {
    const u32 left = c.ids[axis][j], right = c.ids[axis][j+1];
    box.compose(c.boxes[left]);
    const auto larea = box.halfarea(), rarea = c.rlboxes[right].halfarea();
    const auto cost = larea*n + rarea*(primnum-n);
    n++;
    if (!c.istri[left]) alltris = false;
    if (cost > part.cost) continue;
    part.cost = cost;
    part.last[ONLEFT] = j;
    part.first[ONRIGHT] = j + 1;
    part.boxes[ONLEFT] = box;
    part.boxes[ONRIGHT] = c.rlboxes[c.ids[axis][j + 1]];
  }

  // if there is a box, we do not try to make a leaf from this node since we
  // want to have one box per leaf only
  const u32 id = c.ids[axis][last];
  if (!alltris || !c.istri[id]) return part;

  // get the real cost (with takes into account traversal and intersection)
  box.compose(c.boxes[id]);
  const auto harea = box.halfarea();
  part.cost *= sahintersectioncost;
  part.cost += sahtraversalcost * harea;
  if (primnum > maxprimitivenum) return part;

  // test the last partition where all primitives are inside one node
  const auto cost = sahintersectioncost*primnum*harea;
  if (cost <= part.cost) {
    part.cost = cost;
    part.last[ONRIGHT]  = part.last[ONLEFT]  = -1;
    part.first[ONRIGHT] = part.first[ONLEFT] = -1;
    part.boxes[ONRIGHT] = part.boxes[ONLEFT] = box;
  }
  return part;
}

struct segment {
  segment(void) {}
  segment(s32 first, s32 last, u32 id, const aabb &box) :
    first(first), last(last), id(id), box(box) {}
  s32 first, last;
  u32 id;
  aabb box;
};

INLINE void maketriangle(const primitive &t, waldtriangle &w, u32 id, u32 matid) {
  const vec3f &A(t.v[0]), &B(t.v[1]), &C(t.v[2]);
  const vec3f b(B-A), c(C-A), N(cross(b,c));
  u32 k = 0;
  for (u32 i=1; i<3; ++i) k = abs(N[i]) > abs(N[k]) ? i : k;
  const u32 u = (k+1)%3, v = (k+2)%3;
  const float denom = (b[u]*c[v] - b[v]*c[u]), krec = N[k];
  w.n = vec2f(N[u]/krec, N[v]/krec);
  w.bn = vec2f(-b[v]/denom, b[u]/denom);
  w.cn = vec2f(c[v]/denom, -c[u]/denom);
  w.vertk = vec2f(A[u], A[v]);
  w.nd = dot(N,A)/krec;
  w.id = id;
  w.k = k;
  w.sign = N[k] < 0.f ? 1 : 0;
  w.matid = matid;
}

INLINE void makenode(compiler &c, const segment &data, u32 axis) {
  c.root[data.id].box = data.box;
  c.root[data.id].setflag(intersector::NONLEAF);
  c.root[data.id].setaxis(axis);
  c.root[data.id].setoffset(c.currid+1-data.id);
  c.nodenum++;
}

INLINE void makeleaf(compiler &c, const segment &data) {
  const auto n = data.last - data.first + 1;
  const auto &first = c.prims[c.ids[0][data.first]];
  auto &node = c.root[data.id];
  node.box = data.box;
  if (first.type == primitive::INTERSECTOR) {
    assert(n==1);
    node.setflag(intersector::ISECLEAF);
    node.setptr(first.isec);
  } else {
    node.setflag(intersector::TRILEAF);
    node.setptr(&c.acc[c.accnum]);
    for (auto j = data.first; j <= data.last; ++j) {
      const auto id = c.ids[0][j];
      assert(c.prims[id].type == primitive::TRI);
      maketriangle(c.prims[id], c.acc[c.accnum], id, 0);
      c.acc[c.accnum++].num = n; // encode number of prims in each triangle
    }
  }
  c.leafnum++;
  c.nodenum++;
}

INLINE void growboxes(compiler &c) {
  const float aabbeps = 1e-6f;
  loopi(2*c.n-1) {
    c.root[i].box.pmin = c.root[i].box.pmin - vec3f(aabbeps);
    c.root[i].box.pmax = c.root[i].box.pmax + vec3f(aabbeps);
  }
}

void compiler::compile(void) {
  segment node;
  segment stack[64];
  u32 stacksz = 1;
  stack[0] = segment(0,n-1,0,scenebox);

  while (stacksz) {
    node = stack[--stacksz];
    for (;;) {

      // we are done and we make a leaf
      if (node.last-node.first == 0) {
        makeleaf(*this, node);
        break;
      }

      // find the best partition for this node
      partition best = sweep<0>(*this, node.first, node.last);
      partition part = sweep<1>(*this, node.first, node.last);
      if (part.cost < best.cost) best = part;
      part = sweep<2>(*this, node.first, node.last);
      if (part.cost < best.cost) best = part;

      // The best partition is actually *no* partition: we make a leaf
      if (best.first[ONLEFT] == -1) {
        makeleaf(*this, node);
        break;
      }

      // register this node
      makenode(*this, node, best.axis);

      // first, store the positions of the primitives
      for (int j = best.first[ONLEFT]; j <= best.last[ONLEFT]; ++j)
        pos[ids[best.axis][j]] = ONLEFT;
      for (int j = best.first[ONRIGHT]; j <= best.last[ONRIGHT]; ++j)
        pos[ids[best.axis][j]] = ONRIGHT;

      // then, for each axis, reorder the indices for the next step
      int leftnum, rightnum;
      loopi(OTHERAXISNUM) {
        const int otheraxis[] = {1,2,0,1};
        const int d0 = otheraxis[best.axis + i];
        leftnum = 0, rightnum = 0;
        for (int j = node.first; j <= node.last; ++j)
          if (pos[ids[d0][j]] == ONLEFT)
            ids[d0][node.first + leftnum++] = ids[d0][j];
          else
            tmpids[rightnum++] = ids[d0][j];
        for (int j = node.first + leftnum; j <= node.last; ++j)
          ids[d0][j] = tmpids[j - leftnum - node.first];
      }

      // prepare the stack data for the next step
      const int p0 = rightnum > leftnum ? ONLEFT : ONRIGHT;
      const int p1 = rightnum > leftnum ? ONRIGHT : ONLEFT;
      stack[stacksz++] = segment(best.first[p1], best.last[p1], currid+p1+1, best.boxes[p1]);
      node.first = best.first[p0];
      node.last = best.last[p0];
      node.box = best.boxes[p0];
      node.id = currid+p0+1;
      currid += 2;
    }
  }

  growboxes(*this);
}

intersector *create(const primitive *prims, int n) {
  if (n==0) return NULL;
  compiler c;
  auto tree = NEWE(intersector);
  c.injection(prims, n);
  c.compile();
  c.acc.moveto(tree->acc);
  tree->root = c.root;
  if (bvhstatitics) {
    con::out("bvh: %d nodes %d leaves", c.nodenum, c.leafnum);
    con::out("bvh: %f triangles/leaf", float(n) / float(c.leafnum));
  }
  return tree;
}

void destroy(intersector *bvhtree) {
  if (bvhtree == NULL) return;
  SAFE_DELA(bvhtree->root);
  SAFE_DEL(bvhtree);
}

static const u32 waldmodulo[] = {1,2,0,1};

template <bool occludedonly>
INLINE bool raytriangle(const waldtriangle &tri, vec3f org, vec3f dir, hit *hit = NULL) {
  const u32 k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
  const vec2f dirk(dir[ku], dir[kv]);
  const vec2f posk(org[ku], org[kv]);
  const float t = (tri.nd-org[k]-dot(tri.n,posk))/(dir[k]+dot(tri.n,dirk));
  if (!((hit->t > t) & (t >= 0.f)))
    return false;
  const vec2f h = posk + t*dirk - tri.vertk;
  const float beta = dot(h,tri.bn), gamma = dot(h,tri.cn);
  if ((beta < 0.f) | (gamma < 0.f) | ((beta + gamma) > 1.f)) return false;
  hit->t = t;
  if (!occludedonly) {
    hit->u = beta;
    hit->v = gamma;
    hit->id = tri.id;
    hit->n[k] = tri.sign ? -1.f : 1.f;
    hit->n[waldmodulo[k]] = hit->n[k]*tri.n.x;
    hit->n[waldmodulo[k+1]] = hit->n[k]*tri.n.y;
  }
  return true;
}

void closest(const intersector &bvhtree, const ray &r, hit &hit) {
  const s32 signs[3] = {(r.dir.x>=0.f)&1, (r.dir.y>=0.f)&1, (r.dir.z>=0.f)&1};
  const auto rdir = rcp(r.dir);
  const intersector::node *stack[64];
  stack[0] = bvhtree.root;
  u32 stacksz = 1;

  while (stacksz) {
    const intersector::node *node = stack[--stacksz];
    for (;;) {
      auto res = slab(node->box, r.org, rdir, hit.t);
      if (!res.isec) break;
    processnode:
      const u32 flag = node->getflag();
      if (flag == intersector::NONLEAF) {
        const s32 farindex = signs[node->getaxis()];
        const s32 nearindex = farindex^1;
        const u32 offset = node->getoffset();
        stack[stacksz++] = node+offset+farindex;
        node = node+offset+nearindex;
      } else {
        if (flag == intersector::TRILEAF) {
          auto tris = node->getptr<waldtriangle>();
          const s32 n = tris->num;
          loopi(n) raytriangle<false>(tris[i], r.org, r.dir, &hit);
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}

bool occluded(const intersector &bvhtree, const ray &r) {
  const intersector::node *stack[64];
  const auto rdir = rcp(r.dir);
  stack[0] = bvhtree.root;
  u32 stacksz = 1;

  while (stacksz) {
    const intersector::node *node = stack[--stacksz];
    for (;;) {
      auto res = slab(node->box, r.org, rdir, r.tfar);
      if (!res.isec) break;
    processnode:
      const u32 flag = node->getflag();
      if (flag == intersector::NONLEAF) {
        const u32 offset = node->getoffset();
        stack[stacksz++] = node+offset+1;
        node = node+offset;
      } else {
        if (flag == intersector::TRILEAF) {
          auto tris = node->getptr<waldtriangle>();
          const s32 n = tris->num;
          loopi(n) if (raytriangle<true>(tris[i], r.org, r.dir)) return true;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
        break;
      }
    }
  }
  return false;
}

INLINE isecres slabfirst(const aabb &box, const raypacket &p,
                         const vec3f *rdir, u32 &first,
                         const packethit &hit)
{
  for (; first < p.raynum; ++first) {
    const auto res = slab(box, p.org(first), rdir[first], hit[first].t);
    if (res.isec) return res;
  }
  return isecres(false);
}

INLINE isecres slabone(const aabb &box, const raypacket &p,
                       const vec3f *rdir, u32 first,
                       const packethit &hit)
{
  return slab(box, p.org(first), rdir[first], hit[first].t);
}

INLINE void slaball(const aabb &box, const raypacket &p,
                    const vec3f *rdir, u32 first,
                    packethit &hit)
{
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(box, p.org(rayid), rdir[rayid], hit[rayid].t);
    if (res.isec) {
      hit[rayid].t = res.t;
      hit[rayid].id = 0;
    }
  }
}

INLINE void slabfilter(const aabb &box, const raypacket &p,
                       const vec3f *rdir, u32 *active, u32 first,
                       const packethit &hit)
{
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(box, p.org(rayid), rdir[rayid], hit[rayid].t);
    active[rayid] = res.isec?1:0;
  }
}

INLINE bool cullia(const aabb &box, const raypacket &p) {
  assert(false && "Not implemented");
  return false;
}

INLINE isecres slabfirstco(const aabb &box, const raypacket &p,
                           const vec3f *rdir, u32 &first,
                           const packethit &hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  for (; first < p.raynum; ++first) {
    const auto res = slab(pmin, pmax, rdir[first], hit[first].t);
    if (res.isec) return res;
  }
  return isecres(false);
}

INLINE isecres slaboneco(const aabb &box, const raypacket &p,
                         const vec3f *rdir, u32 first,
                         const packethit &hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  return slab(pmin, pmax, rdir[first], hit[first].t);
}

INLINE void slaballco(const aabb &box, const raypacket &p,
                      const vec3f *rdir, u32 first,
                      packethit &hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(pmin, pmax, rdir[rayid], hit[rayid].t);
    if (res.isec) {
      hit[rayid].t = res.t;
      hit[rayid].id = 0;
    }
  }
}

INLINE void slabfilterco(const aabb &box, const raypacket &p,
                         const vec3f *rdir, u32 *active,
                         u32 first, const packethit &hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(pmin, pmax, rdir[rayid], hit[rayid].t);
    active[rayid] = res.isec?1:0;
  }
}

INLINE bool culliaco(const aabb &box, const raypacket &p) {
  const vec3f pmin = box.pmin - p.org();
  const vec3f pmax = box.pmax - p.org();
  const auto txyz = makeinterval(pmin,pmax)*p.iardir;
  return empty(I(txyz.x,txyz.y,txyz.z,intervalf(0.f,FLT_MAX)));
}

template <u32 flags>
NOINLINE void closestinternal(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  const s32 signs[3] = {(p.dir().x>=0.f)&1, (p.dir().y>=0.f)&1, (p.dir().z>=0.f)&1};
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;
  vec3f rdir[raypacket::MAXRAYNUM];
  loopi(s32(p.raynum)) rdir[i] = rcp(p.dir(i));

  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      isecres res(false);
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, rdir, first, hit);
          if (res.isec) goto processnode;
          if (culliaco(node->box, p)) break;
        } else {
          res = slabone(node->box, p, rdir, first, hit);
          if (res.isec) goto processnode;
          if (cullia(node->box, p)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, rdir, first, hit);
      else
        res = slabfirst(node->box, p, rdir, first, hit);
      if (!res.isec) break;
    processnode:
      const u32 flag = node->getflag();
      if (flag == intersector::NONLEAF) {
        const s32 farindex = signs[node->getaxis()];
        const s32 nearindex = farindex^1;
        const u32 offset = node->getoffset();
        stack[stacksz++] = makepair(node+offset+farindex, first);
        node = node+offset+nearindex;
      } else {
        if (flag == intersector::TRILEAF) {
          auto tris = node->getptr<waldtriangle>();
          const s32 n = tris->num;
          u32 active[raypacket::MAXRAYNUM];
          active[first] = 1;
          if (flags & raypacket::COMMONORG)
            slabfilterco(node->box, p, rdir, active, first+1, hit);
          else
            slabfilter(node->box, p, rdir, active, first+1, hit);
          loopi(n) rangej(first,p.raynum)
            if (active[j]) raytriangle<false>(tris[i], p.org(j), p.dir(j), hit+j);
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}
#define CASE(X) case X: closestinternal<X>(bvhtree, p, hit); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void closest(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  switch (p.flags) {
    CASE4(0)
    CASE4(4)
    CASE4(8)
    CASE4(12)
  };
}
#undef CASE
#undef CASE4

template <u32 flags>
NOINLINE void occludedinternal(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;
  vec3f rdir[raypacket::MAXRAYNUM];
  loopi(s32(p.raynum)) rdir[i] = rcp(p.dir(i));
  u8 occluded[raypacket::MAXRAYNUM];
  u32 occludednum = 0;
  loopi(p.raynum) occluded[i] = 0;
  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      isecres res(false);
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, rdir, first, hit);
          if (res.isec) goto processnode;
          if (culliaco(node->box, p)) break;
        } else {
          res = slabone(node->box, p, rdir, first, hit);
          if (res.isec) goto processnode;
          if (cullia(node->box, p)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, rdir, first, hit);
      else
        res = slabfirst(node->box, p, rdir, first, hit);
      if (!res.isec) break;
    processnode:
      const u32 flag = node->getflag();
      if (flag == intersector::NONLEAF) {
        const u32 offset = node->getoffset();
        stack[stacksz++] = makepair(node+offset+1, first);
        node = node+offset;
      } else {
        if (flag == intersector::TRILEAF) {
          auto tris = node->getptr<waldtriangle>();
          const s32 n = tris->num;
          u32 active[raypacket::MAXRAYNUM];
          active[first] = 1;
          if (flags & raypacket::COMMONORG)
            slabfilterco(node->box, p, rdir, active, first+1, hit);
          else
            slabfilter(node->box, p, rdir, active, first+1, hit);
          loopi(n) rangej(first,p.raynum)
            if (!occluded[j] && active[j] && raytriangle<true>(tris[i], p.org(j), p.dir(j), hit+j)) {
              hit[j].id = tris[i].id;
              occluded[j] = 1;
              occludednum++;
            }
          if (occludednum == p.raynum) return;
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}
#define CASE(X) case X: occludedinternal<X>(bvhtree, p, hit); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void occluded(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  switch (p.flags) {
    CASE4(0)
    CASE4(4)
    CASE4(8)
    CASE4(12)
  };
}
#undef CASE
#undef CASE4
} /* namespace bvh */
} /* namespace q */

