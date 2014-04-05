/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhsimd.cpp -> implements intersection routines using SIMD instructions
 -------------------------------------------------------------------------*/
#include <cstring>
#include <cfloat>
#include <cmath>
#include "base/math.hpp"
#include "base/sse.hpp"
#include "base/avx.hpp"
#include "bvh.hpp"
#include "bvhinternal.hpp"

namespace q {
namespace rt {
namespace NAMESPACE {
struct raypacketextra {
  array3f rdir;                    // used by ray/box intersection
  interval3f iaorg, iadir, iardir; // only used when INTERVALARITH is set
  float iaminlen, iamaxlen;        // only used when INTERVALARITH is set
};
static const u32 waldmodulo[] = {1,2,0,1};
INLINE bool cullia(const aabb &box, const raypacket &p, const raypacketextra &extra) {
  assert(false && "Not implemented");
  return false;
}

INLINE bool culliaco(const aabb &box, const raypacket &p, const raypacketextra &extra) {
  const vec3f pmin = box.pmin - p.sharedorg;
  const vec3f pmax = box.pmax - p.sharedorg;
  const auto txyz = makeinterval(pmin,pmax)*extra.iardir;
  return empty(I(txyz.x,txyz.y,txyz.z,intervalf(extra.iaminlen,extra.iamaxlen)));
}

#if defined(__AVX__)
typedef avxf soaf;
typedef avxb soab;
INLINE void store(void *ptr, const soaf &x) {store8f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store8b(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store8f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {
  const auto p = shuffle<0,0>(v);
  const auto s = shuffle<0,0,0,0>(p,p);
  return s;
}
#elif defined(__SSE__)
typedef ssef soaf;
typedef sseb soab;
INLINE void store(void *ptr, const soaf &x) {store4f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store4b(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store4f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {return shuffle<0,0,0,0>(v,v);}
#else
#error "unsupported SIMD"
#endif

typedef vec3<soaf> soa3f;
typedef vec2<soaf> soa2f;

INLINE soaf get(const arrayf &x, u32 idx) {
  return soaf::load(&x[idx*soaf::size]);
}

INLINE soa3f get(const array3f &v, u32 idx) {
  const soaf x = get(v[0], idx);
  const soaf y = get(v[1], idx);
  const soaf z = get(v[2], idx);
  return soa3f(x,y,z);
}

INLINE void set(array3f &out, const soa3f &v, u32 idx) {
  store(&out[0][idx*soaf::size], v.x);
  store(&out[1][idx*soaf::size], v.y);
  store(&out[2][idx*soaf::size], v.z);
}

struct soaisec {
  INLINE soaisec(const soab &isec, const soaf &t) : t(t), isec(isec) {}
  soaf t;
  soab isec;
};

INLINE soaisec slab(const soa3f &pmin, const soa3f &pmax, const soa3f &rdir, const soaf &t) {
  const auto l1 = pmin*rdir;
  const auto l2 = pmax*rdir;
  const auto tfar = reducemin(max(l1,l2));
  const auto tnear = reducemax(min(l1,l2));
  const auto isec = (tfar >= tnear) & (tfar >= soaf(zero)) & (tnear < t);
  const auto tisec = max(soaf(zero),tnear);
  return soaisec(isec, tisec);
}

INLINE bool slabfirst(const aabb &RESTRICT box,
                      const raypacket &RESTRICT p,
                      const raypacketextra &RESTRICT extra,
                      u32 &RESTRICT first,
                      const arrayf &RESTRICT hit)
{
  auto const packetnum = p.raynum / soaf::size;
  for (; first < packetnum; ++first) {
    const auto org = get(p.vorg,first);
    const auto rd = get(extra.rdir,first);
    const auto t = get(hit,first);
    const auto pmin = soa3f(box.pmin)-org;
    const auto pmax = soa3f(box.pmax)-org;
    const auto res = slab(pmin, pmax, rd, t);
    if (any(res.isec)) return true;
  }
  return false;
}

INLINE bool slabone(const aabb &RESTRICT box,
                    const raypacket &RESTRICT p,
                    const raypacketextra &RESTRICT extra,
                    u32 first,
                    const arrayf &RESTRICT hit)
{
  const auto org = get(p.vorg,first);
  const auto rd = get(extra.rdir,first);
  const auto t = get(hit,first);
  const auto pmin = soa3f(box.pmin)-org;
  const auto pmax = soa3f(box.pmax)-org;
  return any(slab(pmin, pmax, rd, t).isec);
}

INLINE void slabfilter(const aabb &RESTRICT box,
                       const raypacket &RESTRICT p,
                       const raypacketextra &RESTRICT extra,
                       u32 *RESTRICT active,
                       u32 first,
                       const arrayf &RESTRICT hit)
{
  auto const packetnum = p.raynum / soaf::size;
  for (u32 id = first; id < packetnum; ++id) {
    const auto org = get(p.vorg,id);
    const auto rd = get(extra.rdir,id);
    const auto t = get(hit,id);
    const auto pmin = soa3f(box.pmin)-org;
    const auto pmax = soa3f(box.pmax)-org;
    const auto res = slab(pmin, pmax, rd, t);
    active[id] = any(res.isec);
  }
}

INLINE bool slabfirstco(const aabb &RESTRICT box,
                        const raypacket &RESTRICT p,
                        const raypacketextra &RESTRICT extra,
                        u32 &RESTRICT first,
                        const arrayf &RESTRICT hit)
{
  const auto packetnum = p.raynum / soaf::size;
  const auto pmin = soa3f(box.pmin - p.sharedorg);
  const auto pmax = soa3f(box.pmax - p.sharedorg);
  for (; first < packetnum; ++first) {
    const auto rd = get(extra.rdir,first);
    const auto t = get(hit,first);
    if (any(slab(pmin, pmax, rd, t).isec)) return true;
  }
  return false;
}

INLINE bool slaboneco(const aabb &RESTRICT box,
                      const raypacket &RESTRICT p,
                      const raypacketextra &RESTRICT extra,
                      u32 first,
                      const arrayf &RESTRICT hit)
{
  const auto pmin = soa3f(box.pmin - p.sharedorg);
  const auto pmax = soa3f(box.pmax - p.sharedorg);
  const auto rd = get(extra.rdir,first);
  const auto t = get(hit,first);
  return any(slab(pmin, pmax, rd, t).isec);
}

INLINE void slabfilterco(const aabb &RESTRICT box,
                         const raypacket &RESTRICT p,
                         const raypacketextra &RESTRICT extra,
                         u32 *RESTRICT active,
                         u32 first,
                         const arrayf &RESTRICT hit)
{
  const auto packetnum = p.raynum / soaf::size;
  const auto pmin = soa3f(box.pmin - p.sharedorg);
  const auto pmax = soa3f(box.pmax - p.sharedorg);
  for (u32 rayid = first; rayid < packetnum; ++rayid) {
    const auto rd = get(extra.rdir,rayid);
    const auto t = get(hit,rayid);
    const auto res = slab(pmin, pmax, rd, t);
    active[rayid] = any(res.isec);
  }
}

INLINE float asfloat(u32 x) {union {float f; u32 u;}; u = x; return f;}

template <bool sharedorg>
INLINE soa3f getorg(const raypacket &p, u32 idx) {
  return get(p.vorg, idx);
}
template <> INLINE
soa3f getorg<true>(const raypacket &p, u32 idx) {
  return soa3f(p.sharedorg);
}
template <bool shareddir>
INLINE soa3f getdir(const raypacket &p, u32 idx) {
  return get(p.vdir, idx);
}
template <> INLINE
soa3f getdir<true>(const raypacket &p, u32 idx) {
  return soa3f(p.shareddir);
}

template <u32 flags>
INLINE void closest(const waldtriangle &RESTRICT tri,
                    const raypacket &RESTRICT p,
                    const u32 *RESTRICT active,
                    u32 first,
                    packethit &RESTRICT hit)
{
  const auto packetnum = p.raynum/soaf::size;
  const auto trind = soaf(tri.nd);
  const auto trin = soa2f(tri.n);
  const auto k = u32(tri.k), ku = waldmodulo[k], kv = waldmodulo[k+1];
  rangej(first,packetnum) if (active[j]) {
    const auto idx = j*soaf::size;
    const auto raydir = getdir<flags&raypacket::SHAREDDIR>(p,j);
    const auto rayorg = getorg<flags&raypacket::SHAREDORG>(p,j);
    const auto dist = get(hit.t, j);
    const soa2f dir(raydir[ku], raydir[kv]);
    const soa2f org(rayorg[ku], rayorg[kv]);

    // evalute intersection with triangle plane
    const auto t = (trind-rayorg[k]-dot(trin,org))/(raydir[k]+dot(trin,dir));
    const auto tmask = (t<dist) & (t>soaf(zero));
    if (none(tmask)) continue;

    // evaluate aperture
    const auto trivertk = soa2f(tri.vertk);
    const auto tribn = soa2f(tri.bn);
    const auto tricn = soa2f(tri.cn);
    const auto h = org + t*dir - trivertk;
    const auto u = dot(h,tribn);
    const auto v = dot(h,tricn);
    const auto aperture = (u>=soaf(zero)) & (v>=soaf(zero)) & (u+v<=soaf(one));
    const auto m = aperture & tmask;
    if (none(m)) continue;

    // we are good to go. we can update the hit point
    const auto triid = soaf::broadcast(&tri.id);
    const auto sign = soaf(asfloat(tri.sign<<31u));
    maskstore(m,&hit.t[idx], t);
    maskstore(m,&hit.u[idx], u);
    maskstore(m,&hit.v[idx], v);
    maskstore(m,&hit.id[idx], triid);
    maskstore(m,&hit.n[k][idx],soaf(one)^sign);
    maskstore(m,&hit.n[ku][idx],tri.n.x^sign);
    maskstore(m,&hit.n[kv][idx],tri.n.y^sign);
  }
}

template <u32 flags>
INLINE u32 occluded(const waldtriangle &RESTRICT tri,
                    const raypacket &RESTRICT p,
                    const u32 *RESTRICT active,
                    u32 first,
                    packetshadow &RESTRICT s)
{
  const auto packetnum = p.raynum/soaf::size;
  const auto trind = soaf(tri.nd);
  const auto trin = soa2f(tri.n);
  const auto k = u32(tri.k), ku = waldmodulo[k], kv = waldmodulo[k+1];
  auto occludednum = 0u;
  rangej(first,packetnum) if (active[j]) {
    const auto idx = j*soaf::size;
    const auto raydir = getdir<flags&raypacket::SHAREDDIR>(p,j);
    const auto rayorg = getorg<flags&raypacket::SHAREDORG>(p,j);
    const auto dist = get(s.t, j);
    const soa2f dir(raydir[ku], raydir[kv]);
    const soa2f org(rayorg[ku], rayorg[kv]);

    // evalute intersection with triangle plane
    const auto t = (trind-rayorg[k]-dot(trin,org))/(raydir[k]+dot(trin,dir));
    const auto tmask = (t<dist) & (t>soaf(zero));
    if (none(tmask)) continue;

    // evaluate aperture
    const auto trivertk = soa2f(tri.vertk);
    const auto tribn = soa2f(tri.bn);
    const auto tricn = soa2f(tri.cn);
    const auto oldoccluded = soab::load(&s.occluded[idx]);
    const auto h = org + t*dir - trivertk;
    const auto u = dot(h,tribn);
    const auto v = dot(h,tricn);
    const auto aperture = (u>=soaf(zero)) & (v>=soaf(zero)) & (u+v<=soaf(one));
    const auto m = andnot(oldoccluded, aperture&tmask);
    if (none(m)) continue;
    occludednum += popcnt(m);
    store(&s.occluded[idx], oldoccluded|m);
  }
  return occludednum;
}

INLINE bool samesign(const soa3f &min, const soa3f &max, vec3f &mindir, vec3f &maxdir) {
  const soa3f minall(vreduce_min(min.x),vreduce_min(min.y),vreduce_min(min.z));
  const soa3f maxall(vreduce_max(max.x),vreduce_max(max.y),vreduce_max(max.z));
  const soa3f mulall = minall*maxall;
  const auto signx = movemask(soab(mulall.x));
  const auto signy = movemask(soab(mulall.y));
  const auto signz = movemask(soab(mulall.z));
  if ((signx|signy|signz) == 0) {
    mindir = vec3f(fextract<0>(minall.x),fextract<0>(minall.y),fextract<0>(minall.z));
    maxdir = vec3f(fextract<0>(maxall.x),fextract<0>(maxall.y),fextract<0>(maxall.z));
    return true;
  } else
    return false;
}

template <typename Hit>
INLINE u32 initextra(raypacketextra &RESTRICT extra,
                     const raypacket &RESTRICT p,
                     const Hit &RESTRICT hit)
{

  // build extra structures required by the bvh intersection
  soa3f soamindir(FLT_MAX), soamaxdir(-FLT_MAX);
  const auto packetnum = p.raynum/soaf::size;
  loopi(packetnum) {
    const auto dir = get(p.vdir, i);
    const auto r = soaf(one) / dir;
    soamindir = min(soamindir, dir);
    soamaxdir = max(soamaxdir, dir);
    set(extra.rdir, r, i);
  }

  vec3f mindir, maxdir;
  if (samesign(soamindir, soamaxdir, mindir, maxdir)) {
    if ((p.flags & raypacket::SHAREDORG) == 0) {
      vec3f minorg(FLT_MAX), maxorg(-FLT_MAX);
      loopi(p.raynum) {
        minorg = min(minorg, p.org(i));
        maxorg = max(maxorg, p.org(i));
      }
      extra.iaorg = makeinterval(minorg, maxorg);
    } else
      extra.iaorg = makeinterval(p.sharedorg, p.sharedorg);
    extra.iadir = makeinterval(mindir, maxdir);
    extra.iardir = rcp(extra.iadir);
    if (typeequal<Hit,packetshadow>::value) {
      extra.iamaxlen = -FLT_MAX;
      extra.iaminlen = FLT_MAX;
      loopi(p.raynum)
        extra.iamaxlen = max(extra.iamaxlen, hit.t[i]);
    } else
      extra.iamaxlen = FLT_MAX;
    extra.iaminlen = 0.f;
    return p.flags | raypacket::INTERVALARITH;
  } else
    return p.flags;
}

template <u32 flags>
void closest(const intersector &RESTRICT bvhtree,
             const raypacket &RESTRICT p,
             const raypacketextra &RESTRICT extra,
             packethit &RESTRICT hit)
{
  assert(p.raynum%soaf::size == 0);
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
        if (flags & raypacket::SHAREDORG) {
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
      if (flags & raypacket::SHAREDORG)
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
          u32 active[MAXRAYNUM/soaf::size];
          active[first] = 1;
          if (flags & raypacket::SHAREDORG)
            slabfilterco(node->box, p, extra, active, first+1, hit.t);
          else
            slabfilter(node->box, p, extra, active, first+1, hit.t);
          loopi(n) closest<flags>(tris[i], p, active, first, hit);
          break;
        } else {
          node = node->getptr<intersector>()->root;
          goto processnode;
        }
      }
    }
  }
}

#define CASE(X) case X: closest<X>(bvhtree, p, extra, hit); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void closest(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  assert(p.raynum % soaf::size == 0);

  // build the extra data structures we need to intersect the bvh
  CACHE_LINE_ALIGNED raypacketextra extra;
  const auto flags = initextra(extra, p, hit);
  switch (flags) {
    CASE4(0)
    CASE4(4)
    CASE4(8)
    CASE4(12)
    default: assert(false && "unreachable code"); break;
  };
}
#undef CASE
#undef CASE4

template <u32 flags>
void occludedinternal(const intersector &RESTRICT bvhtree,
                      const raypacket &RESTRICT p,
                      const raypacketextra &RESTRICT extra,
                      packetshadow &RESTRICT s)
{
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
        if (flags & raypacket::SHAREDORG) {
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
      if (flags & raypacket::SHAREDORG)
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
          if (flags & raypacket::SHAREDORG)
            slabfilterco(node->box, p, extra, active, first+1, s.t);
          else
            slabfilter(node->box, p, extra, active, first+1, s.t);
          loopi(n) occnum += occluded<flags>(tris[i], p, active, first, s);
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

#define F false
#define T true
#if defined(__AVX__)
static const avxb seqactivemask[] = {
  avxb(T,F,F,F,F,F,F,F),
  avxb(T,T,F,F,F,F,F,F),
  avxb(T,T,T,F,F,F,F,F),
  avxb(T,T,T,T,F,F,F,F),
  avxb(T,T,T,T,T,F,F,F),
  avxb(T,T,T,T,T,T,F,F),
  avxb(T,T,T,T,T,T,T,F),
  avxb(T,T,T,T,T,T,T,T)
};
#elif defined(__SSE__)
static const sseb seqactivemask[] = {
  sseb(F,F,F,F),
  sseb(T,F,F,F),
  sseb(T,T,F,F),
  sseb(T,T,T,F),
  sseb(T,T,T,T)
};
#endif
#undef F
#undef T

INLINE soa3f splat(const soa3f &v) {
  return soa3f(splat(v.x),splat(v.y),splat(v.z));
}
INLINE soa3f select(const soab &m, const soa3f &a, const soa3f &b) {
  return soa3f(select(m,a.x,b.x),select(m,a.x,b.x),select(m,a.x,b.x));
}

#define CASE(X) case X: occludedinternal<X>(bvhtree, p, extra, s); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void occluded(const intersector &bvhtree, const raypacket &p, packetshadow &s) {

  // pad the packet if number of rays is not multiple of soaf::size
  const auto lastsize = p.raynum % soaf::size;
  const auto initialnum = p.raynum;
  if (lastsize != 0) {
    auto &np = const_cast<raypacket&>(p);
    const auto last = p.raynum / soaf::size;
    const auto org = get(p.vorg, last);
    const auto dir = get(p.vdir, last);
    const auto t = get(s.t, last);
    const auto idx = soaf::size*last;
    const auto m = seqactivemask[lastsize];
    set(np.vorg, select(m,org,splat(org)), idx);
    set(np.vdir, select(m,dir,splat(dir)), idx);
    store(&s.occluded[idx], soab(falsev));
    store(&s.t[idx], splat(t));
    np.raynum = (last+1) * soaf::size;
  }

  // build the extra data structures we need to intersect the bvh
  CACHE_LINE_ALIGNED raypacketextra extra;
  const auto flags = initextra(extra, p, s);
  switch (flags) {
    CASE4(0)
    CASE4(4)
    CASE4(8)
    CASE4(12)
    default: assert(false && "unreachable code"); break;
  };

  // be careful and reset the number of rays we initially got in the packet
  // before any padding
  const_cast<raypacket&>(p).raynum = initialnum;
}
#undef CASE
#undef CASE4
//* tester culling en premier
//* enlever le code scalaire dans initextra
//* faire IA en simd
} /* namespace NAMESPACE */
} /* namespace rt */
} /* namespace q */

