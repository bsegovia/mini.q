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
namespace bvh {
namespace NAMESPACE {
static const u32 waldmodulo[] = {1,2,0,1};
INLINE bool cullia(const aabb &box, const raypacket &p) {
  assert(false && "Not implemented");
  return false;
}

INLINE bool culliaco(const aabb &box, const raypacket &p) {
  const vec3f pmin = box.pmin - p.org();
  const vec3f pmax = box.pmax - p.org();
  const auto txyz = makeinterval(pmin,pmax)*p.iardir;
  return empty(I(txyz.x,txyz.y,txyz.z,intervalf(p.iaminlen,p.iamaxlen)));
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
                      const array3f &RESTRICT rdir,
                      u32 &RESTRICT first,
                      const arrayf &RESTRICT hit)
{
  auto const packetnum = p.raynum / soaf::size;
  for (; first < packetnum; ++first) {
    const auto org = get(p.vorg,first);
    const auto rd = get(rdir,first);
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
                    const array3f &RESTRICT rdir,
                    u32 first,
                    const arrayf &RESTRICT hit)
{
  const auto org = get(p.vorg,first);
  const auto rd = get(rdir,first);
  const auto t = get(hit,first);
  const auto pmin = soa3f(box.pmin)-org;
  const auto pmax = soa3f(box.pmax)-org;
  return any(slab(pmin, pmax, rd, t).isec);
}

INLINE void slabfilter(const aabb &RESTRICT box,
                       const raypacket &RESTRICT p,
                       const array3f &RESTRICT rdir,
                       u32 *RESTRICT active,
                       u32 first,
                       const arrayf &RESTRICT hit)
{
  auto const packetnum = p.raynum / soaf::size;
  for (u32 id = first; id < packetnum; ++id) {
    const auto org = get(p.vorg,id);
    const auto rd = get(rdir,id);
    const auto t = get(hit,id);
    const auto pmin = soa3f(box.pmin)-org;
    const auto pmax = soa3f(box.pmax)-org;
    const auto res = slab(pmin, pmax, rd, t);
    active[id] = any(res.isec);
  }
}

INLINE bool slabfirstco(const aabb &RESTRICT box,
                           const raypacket &RESTRICT p,
                           const array3f &RESTRICT rdir,
                           u32 &RESTRICT first,
                           const arrayf &RESTRICT hit)
{
  const auto packetnum = p.raynum / soaf::size;
  const auto pmin = soa3f(box.pmin - p.org());
  const auto pmax = soa3f(box.pmax - p.org());
  for (; first < packetnum; ++first) {
    const auto rd = get(rdir,first);
    const auto t = get(hit,first);
    if (any(slab(pmin, pmax, rd, t).isec)) return true;
  }
  return false;
}

INLINE bool slaboneco(const aabb &RESTRICT box,
                      const raypacket &RESTRICT p,
                      const array3f &RESTRICT rdir,
                      u32 first,
                      const arrayf &RESTRICT hit)
{
  const auto pmin = soa3f(box.pmin - p.org());
  const auto pmax = soa3f(box.pmax - p.org());
  const auto rd = get(rdir,first);
  const auto t = get(hit,first);
  return any(slab(pmin, pmax, rd, t).isec);
}

INLINE void slabfilterco(const aabb &RESTRICT box,
                         const raypacket &RESTRICT p,
                         const array3f &RESTRICT rdir,
                         u32 *RESTRICT active,
                         u32 first,
                         const arrayf &RESTRICT hit)
{
  const auto packetnum = p.raynum / soaf::size;
  const auto pmin = soa3f(box.pmin - p.org());
  const auto pmax = soa3f(box.pmax - p.org());
  for (u32 rayid = first; rayid < packetnum; ++rayid) {
    const auto rd = get(rdir,rayid);
    const auto t = get(hit,rayid);
    const auto res = slab(pmin, pmax, rd, t);
    active[rayid] = any(res.isec);
  }
}

INLINE float asfloat(u32 x) { union {float f; u32 u;}; u = x; return f;}

INLINE void closest(const waldtriangle &tri, const raypacket &p,
                    const u32 *active, u32 first,
                    packethit &hit)
{
  const auto packetnum = p.raynum/soaf::size;
  const auto trind = soaf(tri.nd);
  const auto trin = soa2f(tri.n);
  const auto k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
  rangej(first,packetnum) if (active[j]) {
    const auto idx = j*soaf::size;
    const auto dirk  = soaf::load(&p.vdir[k][idx]);
    const auto dirku = soaf::load(&p.vdir[ku][idx]);
    const auto dirkv = soaf::load(&p.vdir[kv][idx]);
    const auto orgk  = soaf::load(&p.vorg[k][idx]);
    const auto orgku = soaf::load(&p.vorg[ku][idx]);
    const auto orgkv = soaf::load(&p.vorg[kv][idx]);
    const auto dist = get(hit.t, j);
    const soa2f dir(dirku, dirkv);
    const soa2f org(orgku, orgkv);

    // evalute intersection with triangle plane
    const auto t = (trind-orgk-dot(trin,org))/(dirk+dot(trin,dir));
    const auto tmask = (t<dist) & (t>soaf(zero));
    if (none(tmask)) continue;

    // evaluate aperture
    const auto trivertk = soa2f(tri.vertk);
    const auto tribn = soa2f(tri.bn);
    const auto tricn = soa2f(tri.cn);
    const auto h = org + t*dir - trivertk;
    const auto u = dot(h,tribn), v = dot(h,tricn);
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

INLINE u32 occluded(const waldtriangle &tri, const raypacket &p,
                    const u32 *active, u32 first,
                    packetshadow &s)
{
  const auto packetnum = p.raynum/soaf::size;
  const auto trind = soaf(tri.nd);
  const auto trin = soa2f(tri.n);
  const auto k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
  auto occludednum = 0u;
  rangej(first,packetnum) if (active[j]) {
    const auto idx = j*soaf::size;
    const auto dirk  = soaf::load(&p.vdir[k][idx]);
    const auto dirku = soaf::load(&p.vdir[ku][idx]);
    const auto dirkv = soaf::load(&p.vdir[kv][idx]);
    const auto orgk  = soaf::load(&p.vorg[k][idx]);
    const auto orgku = soaf::load(&p.vorg[ku][idx]);
    const auto orgkv = soaf::load(&p.vorg[kv][idx]);
    const auto dist = get(s.t, j);
    const soa2f dir(dirku, dirkv);
    const soa2f org(orgku, orgkv);

    // evalute intersection with triangle plane
    const auto t = (trind-orgk-dot(trin,org))/(dirk+dot(trin,dir));
    const auto tmask = (t<dist) & (t>soaf(zero));
    if (none(tmask)) continue;

    // evaluate aperture
    const auto trivertk = soa2f(tri.vertk);
    const auto tribn = soa2f(tri.bn);
    const auto tricn = soa2f(tri.cn);
    const auto oldoccluded = soab::load(&s.occluded[idx]);
    const auto h = org + t*dir - trivertk;
    const auto u = dot(h,tribn), v = dot(h,tricn);
    const auto aperture = (u>=soaf(zero)) & (v>=soaf(zero)) & (u+v<=soaf(one));
    const auto m = andnot(oldoccluded, aperture&tmask);
    if (none(m)) continue;
    occludednum += popcnt(m);
    store(&s.occluded[idx], oldoccluded|m);
  }
  return occludednum;
}

template <int flags>
void closestinternal(const intersector &bvhtree, const raypacket &p, packethit &hit) {
  assert(p.raynum%soaf::size == 0);
  const int packetnum = p.raynum/soaf::size;
  const s32 signs[3] = {(p.dir().x>=0.f)&1, (p.dir().y>=0.f)&1, (p.dir().z>=0.f)&1};
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;
  CACHE_LINE_ALIGNED array3f rdir;
  loopi(packetnum) {
    const auto r = soaf(one) / get(p.vdir,i);
    store(&rdir[0][i*soaf::size], r.x);
    store(&rdir[1][i*soaf::size], r.y);
    store(&rdir[2][i*soaf::size], r.z);
  }

  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      bool res = false;
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, rdir, first, hit.t);
          if (res) goto processnode;
          if (culliaco(node->box, p)) break;
        } else {
          res = slabone(node->box, p, rdir, first, hit.t);
          if (res) goto processnode;
          if (cullia(node->box, p)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, rdir, first, hit.t);
      else
        res = slabfirst(node->box, p, rdir, first, hit.t);
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
          if (flags & raypacket::COMMONORG)
            slabfilterco(node->box, p, rdir, active, first+1, hit.t);
          else
            slabfilter(node->box, p, rdir, active, first+1, hit.t);
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
void occludedinternal(const intersector &bvhtree, const raypacket &p, packetshadow &s) {
  const auto packetnum = p.raynum / soaf::size;
  pair<intersector::node*,u32> stack[64];
  stack[0] = makepair(bvhtree.root, 0u);
  u32 stacksz = 1;
  CACHE_LINE_ALIGNED array3f rdir;
  loopi(packetnum) {
    const auto r = soaf(one) / get(p.vdir,i);
    store(&rdir[0][i*soaf::size], r.x);
    store(&rdir[1][i*soaf::size], r.y);
    store(&rdir[2][i*soaf::size], r.z);
  }
  u32 occnum = 0;
  while (stacksz) {
    const auto elem = stack[--stacksz];
    auto node = elem.first;
    auto first = elem.second;
    for (;;) {
      bool res = false;
      if (flags & raypacket::INTERVALARITH) {
        if (flags & raypacket::COMMONORG) {
          res = slaboneco(node->box, p, rdir, first, s.t);
          if (res) goto processnode;
          if (culliaco(node->box, p)) break;
        } else {
          res = slabone(node->box, p, rdir, first, s.t);
          if (res) goto processnode;
          if (cullia(node->box, p)) break;
        }
        ++first;
      }
      if (flags & raypacket::COMMONORG)
        res = slabfirstco(node->box, p, rdir, first, s.t);
      else
        res = slabfirst(node->box, p, rdir, first, s.t);
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
            slabfilterco(node->box, p, rdir, active, first+1, s.t);
          else
            slabfilter(node->box, p, rdir, active, first+1, s.t);
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

#define CASE(X) case X: occludedinternal<X>(bvhtree, p, s); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void occluded(const intersector &bvhtree, const raypacket &p, packetshadow &s) {
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
    store(&np.vorg[0][idx], select(m, org.x, splat(org.x)));
    store(&np.vorg[1][idx], select(m, org.y, splat(org.y)));
    store(&np.vorg[2][idx], select(m, org.z, splat(org.z)));
    store(&np.vdir[0][idx], select(m, dir.x, splat(dir.x)));
    store(&np.vdir[1][idx], select(m, dir.y, splat(dir.y)));
    store(&np.vdir[2][idx], select(m, dir.z, splat(dir.z)));
    store(&s.occluded[idx], soab(falsev));
    store(&s.t[idx], splat(t));
    np.raynum = (last+1) * soaf::size;
  }
  switch (p.flags) {
    CASE4(0)
    CASE4(4)
    CASE4(8)
    CASE4(12)
  };
  const_cast<raypacket&>(p).raynum = initialnum;
}
#undef CASE
#undef CASE4
} /* namespace NAMESPACE */
} /* namespace bvh */
} /* namespace q */

