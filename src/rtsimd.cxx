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
#include "rt.hpp"

//TODO * tester culling en premier
//TODO * enlever le code scalaire dans initextra
//TODO * faire IA en simd

namespace q {
namespace rt {
namespace NAMESPACE {
struct raypacketextra {
  array3f rdir;                    // used by ray/box intersection
  interval3f iaorg, iardir; // only used when INTERVALARITH is set
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
typedef avxi soai;
typedef avxb soab;
INLINE void store(void *ptr, const soai &x) {store8i(ptr, x);}
INLINE void store(void *ptr, const soaf &x) {store8f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store8b(ptr, x);}
INLINE void storeu(void *ptr, const soaf &x) {storeu8f(ptr, x);}
INLINE void storent(void *ptr, const soai &x) {store8i_nt(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store8f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {
  const auto p = shuffle<0,0>(v);
  const auto s = shuffle<0,0,0,0>(p,p);
  return s;
}
#elif defined(__SSE__)
typedef ssef soaf;
typedef ssei soai;
typedef sseb soab;
INLINE void store(void *ptr, const soai &x) {store4i(ptr, x);}
INLINE void store(void *ptr, const soaf &x) {store4f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store4b(ptr, x);}
INLINE void storent(void *ptr, const soai &x) {store4i_nt(ptr, x);}
INLINE void storeu(void *ptr, const soaf &x) {storeu4f(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store4f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {return shuffle<0,0,0,0>(v,v);}
#else
#error "unsupported SIMD"
#endif

typedef vec3<soai> soa3i;
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

INLINE bool cornerray(const waldtriangle &RESTRICT tri,
                      const raypacket &RESTRICT p)
{
  const auto trind = ssef(tri.nd);
  const auto trin = vec2<ssef>(tri.n);
  const auto k = u32(tri.k), ku = waldmodulo[k], kv = waldmodulo[k+1];
  const auto raydir = vec3<ssef>(ssef::load(p.crx),ssef::load(p.cry),ssef::load(p.crz));
  const auto rayorg = vec3<ssef>(p.sharedorg);
  const vec2<ssef> dir(raydir[ku], raydir[kv]);
  const vec2<ssef> org(rayorg[ku], rayorg[kv]);

  // evalute intersection with triangle plane
  const auto t = (trind-rayorg[k]-dot(trin,org))/(raydir[k]+dot(trin,dir));
  const auto tmask = (t>ssef(zero));
  if (none(tmask)) return false;

  // evaluate aperture
  const auto trivertk = vec2<ssef>(tri.vertk);
  const auto tribn = vec2<ssef>(tri.bn);
  const auto tricn = vec2<ssef>(tri.cn);
  const auto h = org + t*dir - trivertk;
  const auto u = dot(h,tribn);
  const auto v = dot(h,tricn);
  const auto w = ssef(one)-u-v;
  if ((movemask(sseb(u))==0xf) | (movemask(sseb(v))==0xf) | (movemask(sseb(w))==0xf))
    return false;
  return true;
}

template <u32 flags>
INLINE void closest(const waldtriangle &RESTRICT tri,
                    const raypacket &RESTRICT p,
                    const u32 *RESTRICT active,
                    u32 first,
                    packethit &RESTRICT hit)
{
  if (0!=(flags&raypacket::CORNERRAYS) && !cornerray(tri, p))
    return;
  const auto packetnum = p.raynum/soaf::size;
  const auto trind = soaf(tri.nd);
  const auto trin = soa2f(tri.n);
  const auto k = u32(tri.k), ku = waldmodulo[k], kv = waldmodulo[k+1];
  rangej(first,packetnum) if (active[j]) {
    const auto idx = j*soaf::size;
    const auto raydir = getdir<0!=(flags&raypacket::SHAREDDIR)>(p,j);
    const auto rayorg = getorg<0!=(flags&raypacket::SHAREDORG)>(p,j);
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
    const auto raydir = getdir<0!=(flags&raypacket::SHAREDDIR)>(p,j);
    const auto rayorg = getorg<0!=(flags&raypacket::SHAREDORG)>(p,j);
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

INLINE void iardir(raypacketextra &extra, const soa3f &min, const soa3f &max) {
  const soa3f minall(vreduce_min(min.x),vreduce_min(min.y),vreduce_min(min.z));
  const soa3f maxall(vreduce_max(max.x),vreduce_max(max.y),vreduce_max(max.z));
  const soa3f mulall = minall*maxall;
  const auto signx = movemask(soab(mulall.x));
  const auto signy = movemask(soab(mulall.y));
  const auto signz = movemask(soab(mulall.z));
  const vec3f mindir(fextract<0>(minall.x),fextract<0>(minall.y),fextract<0>(minall.z));
  const vec3f maxdir(fextract<0>(maxall.x),fextract<0>(maxall.y),fextract<0>(maxall.z));
  extra.iardir = rcp(makeinterval(mindir, maxdir));
  if (signx) extra.iardir.x = intervalf(-FLT_MAX, FLT_MAX);
  if (signy) extra.iardir.y = intervalf(-FLT_MAX, FLT_MAX);
  if (signz) extra.iardir.z = intervalf(-FLT_MAX, FLT_MAX);
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

  iardir(extra, soamindir, soamaxdir);
  if ((p.flags & raypacket::SHAREDORG) == 0) {
    vec3f minorg(FLT_MAX), maxorg(-FLT_MAX);
    loopi(p.raynum) {
      minorg = min(minorg, p.org(i));
      maxorg = max(maxorg, p.org(i));
    }
    extra.iaorg = makeinterval(minorg, maxorg);
  } else
    extra.iaorg = makeinterval(p.sharedorg, p.sharedorg);
  if (typeequal<Hit,packetshadow>::value) {
    extra.iamaxlen = -FLT_MAX;
    extra.iaminlen = FLT_MAX;
    loopi(p.raynum) extra.iamaxlen = max(extra.iamaxlen, hit.t[i]);
  } else
    extra.iamaxlen = FLT_MAX;
  extra.iaminlen = 0.f;
  return p.flags | raypacket::INTERVALARITH;
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
void occluded(const intersector &RESTRICT bvhtree,
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
  avxb(F,F,F,F,F,F,F,F),
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
  return soa3f(select(m,a.x,b.x),select(m,a.y,b.y),select(m,a.z,b.z));
}

#define CASE(X) case X: occluded<X>(bvhtree, p, extra, s); break;
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
    set(np.vorg, select(m,org,splat(org)), last);
    set(np.vdir, select(m,dir,splat(dir)), last);
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

/*-------------------------------------------------------------------------
 - generation of packets
 -------------------------------------------------------------------------*/
#if defined(__AVX__)
static const soaf identityf(0.f,1.f,2.f,3.f,4.f,5.f,6.f,7.f);
static const soai identityi(0,1,2,3,4,5,6,7);
static const soaf packetx(0.f,1.f,2.f,3.f,0.f,1.f,2.f,3.f);
static const soaf packety(0.f,0.f,0.f,0.f,1.f,1.f,1.f,1.f);
#elif defined(__SSE__)
static const soaf identityf(0.f,1.f,2.f,3.f);
static const soai identityi(0,1,2,3);
#endif
static const ssef tilecrx(0.f,0.f,float(TILESIZE),float(TILESIZE));
static const ssef tilecry(0.f,float(TILESIZE),0.f,float(TILESIZE));

void visibilitypacket(const camera &RESTRICT cam,
                      raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize)
{
  const auto rw = rcp(float(screensize.x));
  const auto rh = rcp(float(screensize.y));
  const auto xaxis = soa3f(cam.xaxis*rw);
  const auto zaxis = soa3f(cam.zaxis*rh);
  const auto imgplaneorg = soa3f(cam.imgplaneorg);
  u32 idx = 0;
#if defined(__AVX__)
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; y+=2) {
    const auto ydir = (packety+soaf(float(y)))*zaxis;
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size/2, ++idx) {
      const auto xdir = (packetx+soaf(float(x)))*xaxis;
      const auto dir = imgplaneorg+xdir+ydir;
      set(p.vdir, dir, idx);
    }
  }
#else
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; ++y) {
    const auto ydir = soaf(float(y))*zaxis;
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size, ++idx) {
      const auto xdir = (identityf+soaf(float(x)))*xaxis;
      const auto dir = imgplaneorg+xdir+ydir;
      set(p.vdir, dir, idx);
    }
  }
#endif

  const auto crx = tilecrx+ssef(float(tileorg.x));
  const auto cry = tilecry+ssef(float(tileorg.y));
#if defined(__AVX__)
  const auto sseimgplaneorg = vec3<ssef>(cam.imgplaneorg);
  const auto ssexaxis = vec3<ssef>(cam.xaxis*rw);
  const auto ssezaxis = vec3<ssef>(cam.zaxis*rh);
  const auto crdir = sseimgplaneorg+cry*ssezaxis+crx*ssexaxis;
#else
  const auto crdir = imgplaneorg+cry*zaxis+crx*xaxis;
#endif
  store4f(p.crx,crdir.x);
  store4f(p.cry,crdir.y);
  store4f(p.crz,crdir.z);
  p.raynum = TILESIZE*TILESIZE;
  p.sharedorg = cam.org;
  p.flags = raypacket::SHAREDORG|raypacket::CORNERRAYS;
}

void clearpackethit(packethit &hit) {
  const auto packetnum = MAXRAYNUM/soaf::size;
  loopi(packetnum) {
    store(&hit.id[i*soaf::size], soaf(asfloat(~0x0u)));
    store(&hit.t[i*soaf::size],  soaf(FLT_MAX));
  }
}

void primarypoint(const raypacket &RESTRICT p,
                  const packethit &RESTRICT hit,
                  array3f &RESTRICT pos,
                  array3f &RESTRICT nor,
                  arrayi &RESTRICT mask)
{
  assert((p.flags & raypacket::SHAREDORG) != 0);
  assert(p.raynum % soaf::size == 0);
  const auto packetnum = p.raynum / soaf::size;
  loopi(packetnum) {
    const auto noisec = soai(~0x0u);
    const auto m = soai::load(&hit.id[i*soaf::size]) != noisec;
    if (none(m))
     continue;
    const auto t = get(hit.t,i);
    const auto dir = get(p.vdir,i);
    const auto unormal = get(hit.n,i);
    const auto org = soa3f(p.sharedorg);
    const auto normal = normalize(unormal);
    const auto position = org + t*dir + soaf(SHADOWRAYBIAS)*normal;
    store(&mask[i*soaf::size], m);
    set(nor, normal, i);
    set(pos, position, i);
  }
}

void shadowpacket(const array3f &RESTRICT pos,
                  const arrayi &RESTRICT mask,
                  const vec3f &RESTRICT lpos,
                  raypacket &RESTRICT shadow,
                  packetshadow &RESTRICT occluded,
                  int raynum)
{
  assert(raynum % soaf::size == 0);
  const auto packetnum = raynum/soaf::size;
  const auto soalpos = soa3f(lpos);
  int curr = 0;
  loopi(packetnum) {
    const auto idx = i*soaf::size;
    const auto m = soab::load(&mask[idx]);
    if (none(m))
      continue;
    const auto dst = get(pos, i);
    const auto dir = dst-soalpos;
    storeu(&occluded.occluded[curr], soaf(zero));
    storeu(&occluded.t[curr], soaf(one));
    if (all(m)) {
      const auto remapped = soai(curr)+identityi;
      store(&occluded.mapping[idx], remapped);
      storeu(&shadow.vdir[0][curr], dir.x);
      storeu(&shadow.vdir[1][curr], dir.y);
      storeu(&shadow.vdir[2][curr], dir.z);
      curr += soaf::size;
    } else if (none(m)) {
      store(&occluded.mapping[idx], soai(mone));
      continue;
    } else loopj(soaf::size) {
      if (mask[idx+j]==0) {
        occluded.mapping[idx+j] = -1;
        continue;
      }
      occluded.mapping[idx+j] = curr;
      shadow.vdir[0][curr] = dir.x[j];
      shadow.vdir[1][curr] = dir.y[j];
      shadow.vdir[2][curr] = dir.z[j];
      ++curr;
    }
  }
  shadow.sharedorg = lpos;
  shadow.raynum = curr;
  shadow.flags = raypacket::SHAREDORG;
}

/*-------------------------------------------------------------------------
 - framebuffer routines
 -------------------------------------------------------------------------*/
void writenormal(const packethit &RESTRICT hit,
                 const vec2i &RESTRICT tileorg,
                 const vec2i &RESTRICT screensize,
                 int *RESTRICT pixels)
{
  u32 idx = 0;
  const auto w = screensize.x;
#if defined(__AVX__)
  auto yoffset0 = w*tileorg.y;
  auto yoffset1 = w+yoffset0;
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; y+=2, yoffset0+=2*w, yoffset1+=2*w) {
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size/2, ++idx) {
#else
  auto yoffset = w*tileorg.y;
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; ++y, yoffset+=w) {
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size, ++idx) {
#endif
      const auto m = soaf::load(&hit.id[idx*soaf::size]) != soaf(~0x0u);
      const auto n = clamp(normalize(get(hit.n, idx)));
      const auto rgb = soa3i(n*soaf(255.f));
      const auto hitcolor = rgb.x | (rgb.y<<8) | (rgb.z<<16) | soai(0xff000000);
      const auto color = select(m, hitcolor, soai(zero));
#if defined(__AVX__)
      store4i_nt(pixels+yoffset0+x, extract<0>(color));
      store4i_nt(pixels+yoffset1+x, extract<1>(color));
#else
      storent(pixels+yoffset+x, color);
#endif
    }
  }
}

void writendotl(const raypacket &RESTRICT shadow,
                const array3f &nor,
                const packetshadow &RESTRICT occluded,
                const vec2i &RESTRICT tileorg,
                const vec2i &RESTRICT screensize,
                int *RESTRICT pixels)
{
  int idx = 0, curr = 0;
  const auto w = screensize.x;
#if defined(__AVX__)
  auto yoffset0 = w*tileorg.y;
  auto yoffset1 = w+yoffset0;
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; y+=2, yoffset0+=2*w, yoffset1+=2*w) {
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size/2, idx+=soaf::size) {
#else
  auto yoffset = w*tileorg.y;
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; ++y, yoffset+=w) {
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; x+=soaf::size, idx+=soaf::size) {
#endif
      soa3f l;
      soab m;
      if (none(soai::load(&occluded.mapping[idx])==soai(mone))) {
        m = soai::loadu(&occluded.occluded[curr])==soai(zero);
        l.x = soaf::loadu(&shadow.vdir[0][curr]);
        l.y = soaf::loadu(&shadow.vdir[1][curr]);
        l.z = soaf::loadu(&shadow.vdir[2][curr]);
        curr += soaf::size;
      } else loopj(soaf::size) {
        const auto remapped = occluded.mapping[idx+j];
        if (remapped == -1) {
          m[j] = 0;
          continue;
        }
        assert(remapped==curr);
        m[j] = ~occluded.occluded[remapped];
        l.x[j] = shadow.vdir[0][remapped];
        l.y[j] = shadow.vdir[1][remapped];
        l.z[j] = shadow.vdir[2][remapped];
        ++curr;
      }
      const auto n = get(nor, idx/soaf::size);
      const auto shade = select(m, -dot(n, normalize(l)), soaf(zero));
      const auto d = soai(soaf(255.f)*clamp(shade));
      const auto rgb = d | (d<<8) | (d<<16) | 0xff000000;
#if defined(__AVX__)
      store4i_nt(pixels+yoffset0+x, extract<0>(rgb));
      store4i_nt(pixels+yoffset1+x, extract<1>(rgb));
#else
      storent(pixels+yoffset+x, rgb);
#endif
    }
  }
}
} /* namespace NAMESPACE */
} /* namespace rt */
} /* namespace q */

