/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhscalar.cpp -> implements bvh intersection routines using plain C
 -------------------------------------------------------------------------*/
#include <cstring>
#include <cfloat>
#include <cmath>
#include "bvh.hpp"
#include "bvhinternal.hpp"

namespace q {
namespace bvh {

/*-------------------------------------------------------------------------
 - single ray tracing routines
 -------------------------------------------------------------------------*/
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

/*-------------------------------------------------------------------------
 - packet ray tracing routines
 -------------------------------------------------------------------------*/
struct raypacketextra {
  array3f rdir;                    // used by ray/box intersection
  interval3f iaorg, iadir, iardir; // only used when INTERVALARITH is set
  float iaminlen, iamaxlen;        // only used when INTERVALARITH is set
};

INLINE bool cullia(const aabb &box, const raypacket &p, const raypacketextra &extra) {
  assert(false && "Not implemented");
  return false;
}

INLINE vec3f get(const array3f &v, u32 idx) {
  return vec3f(v[0][idx], v[1][idx], v[2][idx]);
}

INLINE bool culliaco(const aabb &box, const raypacket &p, const raypacketextra &extra) {
  const vec3f pmin = box.pmin - p.org();
  const vec3f pmax = box.pmax - p.org();
  const auto txyz = makeinterval(pmin,pmax)*extra.iardir;
  return empty(I(txyz.x,txyz.y,txyz.z,intervalf(extra.iaminlen,extra.iamaxlen)));
}

INLINE bool slabfirst(const aabb &RESTRICT box,
                      const raypacket &RESTRICT p,
                      const raypacketextra &extra,
                      u32 &RESTRICT first,
                      const arrayf &RESTRICT hit)
{
  for (; first < p.raynum; ++first) {
    const auto res = slab(box, p.org(first), get(extra.rdir,first), hit[first]);
    if (res.isec) return true;
  }
  return false;
}

INLINE bool slabone(const aabb &RESTRICT box,
                    const raypacket &RESTRICT p,
                    const raypacketextra &extra,
                    u32 first,
                    const arrayf &RESTRICT hit)
{
  return slab(box, p.org(first), get(extra.rdir,first), hit[first]).isec;
}

INLINE void slabfilter(const aabb &RESTRICT box,
                       const raypacket &RESTRICT p,
                       const raypacketextra &extra,
                       u32 *RESTRICT active,
                       u32 first,
                       const arrayf &RESTRICT hit)
{
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(box, p.org(rayid), get(extra.rdir,rayid), hit[rayid]);
    active[rayid] = res.isec?1:0;
  }
}

INLINE bool slabfirstco(const aabb &RESTRICT box,
                        const raypacket &RESTRICT p,
                        const raypacketextra &extra,
                        u32 &RESTRICT first,
                        const arrayf &RESTRICT hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  for (; first < p.raynum; ++first) {
    const auto res = slab(pmin, pmax, get(extra.rdir,first), hit[first]);
    if (res.isec) return true;
  }
  return false;
}

INLINE bool slaboneco(const aabb &RESTRICT box,
                      const raypacket &RESTRICT p,
                      const raypacketextra &extra,
                      u32 first,
                      const arrayf &RESTRICT hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  return slab(pmin, pmax, get(extra.rdir,first), hit[first]).isec;
}

INLINE void slabfilterco(const aabb &RESTRICT box,
                         const raypacket &RESTRICT p,
                         const raypacketextra &extra,
                         u32 *RESTRICT active,
                         u32 first,
                         const arrayf &RESTRICT hit)
{
  const auto pmin = box.pmin - p.org();
  const auto pmax = box.pmax - p.org();
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(pmin, pmax, get(extra.rdir,rayid), hit[rayid]);
    active[rayid] = res.isec?1:0;
  }
}

INLINE void closest(
  const waldtriangle &tri,
  const raypacket &p,
  const u32 *active,
  u32 first,
  packethit &hit)
{
  rangej(first,p.raynum) if (active[j]) {
    const u32 k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
    const auto org = p.org(j);
    const auto dir = p.dir(j);
    const vec2f dirk(dir[ku], dir[kv]);
    const vec2f posk(org[ku], org[kv]);
    const float t = (tri.nd-org[k]-dot(tri.n,posk))/(dir[k]+dot(tri.n,dirk));
    if (!((hit.t[j] > t) & (t >= 0.f)))
      continue;
    const vec2f h = posk + t*dirk - tri.vertk;
    const float beta = dot(h,tri.bn), gamma = dot(h,tri.cn);
    if ((beta < 0.f) | (gamma < 0.f) | ((beta + gamma) > 1.f))
      continue;
    hit.t[j] = t;
    hit.u[j] = beta;
    hit.v[j] = gamma;
    hit.id[j] = tri.id;
    hit.n[k][j] = tri.sign ? -1.f : 1.f;
    hit.n[waldmodulo[k]][j] = hit.n[k][j]*tri.n.x;
    hit.n[waldmodulo[k+1]][j] = hit.n[k][j]*tri.n.y;
  }
}

INLINE u32 occluded(
  const waldtriangle &tri,
  const raypacket &p,
  const u32 *active,
  u32 first,
  packetshadow &s)
{
  u32 occnum = 0;
  rangej(first,p.raynum) if (!s.occluded[j] && active[j]) {
    const u32 k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
    const auto org = p.org(j);
    const auto dir = p.dir(j);
    const vec2f dirk(dir[ku], dir[kv]);
    const vec2f posk(org[ku], org[kv]);
    const float t = (tri.nd-org[k]-dot(tri.n,posk))/(dir[k]+dot(tri.n,dirk));
    if (!((s.t[j] > t) & (t >= 0.f)))
      continue;
    const vec2f h = posk + t*dirk - tri.vertk;
    const float beta = dot(h,tri.bn), gamma = dot(h,tri.cn);
    if ((beta < 0.f) | (gamma < 0.f) | ((beta + gamma) > 1.f))
      continue;
    s.occluded[j] = 1;
    ++occnum;
  }
  return occnum;
}

template <typename Hit>
INLINE u32 initextra(raypacketextra &extra, const raypacket &p, const Hit &hit) {
  vec3f mindir(FLT_MAX), maxdir(-FLT_MAX);
  loopi(p.raynum) {
    const auto dir = p.dir(i);
    const auto r = rcp(dir);
    mindir = min(mindir, dir);
    maxdir = max(maxdir, dir);
    extra.rdir[0][i] = r.x;
    extra.rdir[1][i] = r.y;
    extra.rdir[2][i] = r.z;
  }
  if (all(gt(mindir*maxdir,vec3f(zero)))) {
    if ((p.flags & raypacket::COMMONORG) == 0) {
      vec3f minorg(FLT_MAX), maxorg(-FLT_MAX);
      loopi(p.raynum) {
        minorg = min(minorg, p.org(i));
        maxorg = max(maxorg, p.org(i));
      }
      extra.iaorg = makeinterval(minorg, maxorg);
    } else
      extra.iaorg = makeinterval(p.orgco, p.orgco);
    extra.iadir = makeinterval(mindir, maxdir);
    extra.iardir = rcp(extra.iadir);
    if (typeequal<Hit,packetshadow>::value) {
      extra.iamaxlen = -FLT_MAX;
      extra.iaminlen = FLT_MAX;
      loopi(p.raynum) {
        extra.iamaxlen = max(extra.iamaxlen, hit.t[i]);
        extra.iaminlen = min(extra.iaminlen, hit.t[i]);
      }
    } else {
      extra.iamaxlen = FLT_MAX;
      extra.iaminlen = 0.f;
    }
    return p.flags | bvh::raypacket::INTERVALARITH;
  } else
    return p.flags;
}

void closest(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  raypacketextra extra;
  const u32 flags = initextra(extra, p, hit);
  const s32 signs[3] = {(p.dir().x>=0.f)&1, (p.dir().y>=0.f)&1, (p.dir().z>=0.f)&1};
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;

  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      bool res = false;
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, extra, first, hit.t);
          if (res) goto processnode;
          if (culliaco(node->box, p, extra)) break;
        } else {
          res = slabone(node->box, p, extra, first, hit.t);
          if (res) goto processnode;
          if (cullia(node->box, p, extra)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, extra, first, hit.t);
      else
        res = slabfirst(node->box, p, extra, first, hit.t);
      if (!res) break;
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
          u32 active[MAXRAYNUM];
          active[first] = 1;
          if (flags & raypacket::COMMONORG)
            slabfilterco(node->box, p, extra, active, first+1, hit.t);
          else
            slabfilter(node->box, p, extra, active, first+1, hit.t);
          loopi(n) closest(tris[i], p, active, first, hit);
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}

void occluded(const intersector &bvhtree, const raypacket &p, packetshadow &s) {
  raypacketextra extra;
  const u32 flags = initextra(extra, p, s);
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;
  u32 occnum = 0;
  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      bool res = false;
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, extra, first, s.t);
          if (res) goto processnode;
          if (culliaco(node->box, p, extra)) break;
        } else {
          res = slabone(node->box, p, extra, first, s.t);
          if (res) goto processnode;
          if (cullia(node->box, p, extra)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, extra, first, s.t);
      else
        res = slabfirst(node->box, p, extra, first, s.t);
      if (!res) break;
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
          u32 active[MAXRAYNUM];
          active[first] = 1;
          if (flags & raypacket::COMMONORG)
            slabfilterco(node->box, p, extra, active, first+1, s.t);
          else
            slabfilter(node->box, p, extra, active, first+1, s.t);
          loopi(n) occnum += occluded(tris[i], p, active, first, s);
          if (occnum == p.raynum) return;
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}
} /* namespace bvh */
} /* namespace q */

