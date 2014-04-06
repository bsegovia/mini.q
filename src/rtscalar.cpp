/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rtscalar.cpp -> visibility ray and shadow ray generation
 -------------------------------------------------------------------------*/
#include "rt.hpp"
#include "bvh.hpp"
#include "bvhinternal.hpp"

namespace q {
namespace rt {

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

INLINE void set(array3f &out, const vec3f &v, u32 idx) {
  out[0][idx]=v.x;
  out[1][idx]=v.y;
  out[2][idx]=v.z;
}

INLINE bool culliaco(const aabb &box, const raypacket &p, const raypacketextra &extra) {
  const vec3f pmin = box.pmin - p.sharedorg;
  const vec3f pmax = box.pmax - p.sharedorg;
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
  const auto pmin = box.pmin - p.sharedorg;
  const auto pmax = box.pmax - p.sharedorg;
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
  const auto pmin = box.pmin - p.sharedorg;
  const auto pmax = box.pmax - p.sharedorg;
  return slab(pmin, pmax, get(extra.rdir,first), hit[first]).isec;
}

INLINE void slabfilterco(const aabb &RESTRICT box,
                         const raypacket &RESTRICT p,
                         const raypacketextra &extra,
                         u32 *RESTRICT active,
                         u32 first,
                         const arrayf &RESTRICT hit)
{
  const auto pmin = box.pmin - p.sharedorg;
  const auto pmax = box.pmax - p.sharedorg;
  for (u32 rayid = first; rayid < p.raynum; ++rayid) {
    const auto res = slab(pmin, pmax, get(extra.rdir,rayid), hit[rayid]);
    active[rayid] = res.isec?1:0;
  }
}

template <bool sharedorg>
INLINE vec3f getorg(const raypacket &p, u32 idx) {
  return get(p.vorg, idx);
}
template <bool shareddir>
INLINE vec3f getdir(const raypacket &p, u32 idx) {
  return get(p.vdir, idx);
}
template <> INLINE vec3f getorg<true>(const raypacket &p, u32 idx) {return p.sharedorg;}
template <> INLINE vec3f getdir<true>(const raypacket &p, u32 idx) {return p.shareddir;}

template <u32 flags>
INLINE void closest(
  const waldtriangle &tri,
  const raypacket &p,
  const u32 *active,
  u32 first,
  packethit &hit)
{
  rangej(first,p.raynum) if (active[j]) {
    const u32 k = tri.k, ku = waldmodulo[k], kv = waldmodulo[k+1];
    const auto org = getorg<0!=(flags&raypacket::SHAREDORG)>(p,j);
    const auto dir = getdir<0!=(flags&raypacket::SHAREDDIR)>(p,j);
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

template <u32 flags>
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
    const auto org = getorg<0!=(flags&raypacket::SHAREDORG)>(p,j);
    const auto dir = getdir<0!=(flags&raypacket::SHAREDDIR)>(p,j);
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
      loopi(p.raynum) {
        extra.iamaxlen = max(extra.iamaxlen, hit.t[i]);
        extra.iaminlen = min(extra.iaminlen, hit.t[i]);
      }
    } else {
      extra.iamaxlen = FLT_MAX;
      extra.iaminlen = 0.f;
    }
    return p.flags | raypacket::INTERVALARITH;
  } else
    return p.flags;
}

template <u32 flags>
static void closest(const intersector &RESTRICT bvhtree,
                    const raypacket &RESTRICT p,
                    const raypacketextra &RESTRICT extra,
                    packethit &RESTRICT hit)
{
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
          u32 active[MAXRAYNUM];
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
static void occluded(const intersector &RESTRICT bvhtree,
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
          const auto n = tris->num;
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

#define CASE(X) case X: occluded<X>(bvhtree, p, extra, s); break;
#define CASE4(X) CASE(X) CASE(X+1) CASE(X+2) CASE(X+3)
void occluded(const intersector &bvhtree, const raypacket &p, packetshadow &s) {
  CACHE_LINE_ALIGNED raypacketextra extra;
  const auto flags = initextra(extra, p, s);
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

/*-------------------------------------------------------------------------
 - generation of packets
 -------------------------------------------------------------------------*/
void visibilitypacket(const camera &RESTRICT cam,
                      raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize)
{
  u32 idx = 0;
  for (u32 y = 0; y < u32(TILESIZE); ++y)
  for (u32 x = 0; x < u32(TILESIZE); ++x, ++idx) {
    const auto ray = cam.generate(screensize.x, screensize.y, tileorg.x+x, tileorg.y+y);
    p.setdir(ray.dir, idx);
  }
  p.raynum = TILESIZE*TILESIZE;
  p.sharedorg = cam.org;
  p.flags = raypacket::SHAREDORG;
}

void shadowpacket(const array3f &RESTRICT pos,
                  const arrayi &RESTRICT mask,
                  const vec3f &RESTRICT lpos,
                  raypacket &RESTRICT shadow,
                  packetshadow &RESTRICT occluded,
                  int raynum)
{
  assert((primary.flags & raypacket::SHAREDORG) != 0);
  int curr = 0;
  loopi(raynum) {
    if (u32(mask[i]) != 0) {
      const auto dst = get(pos, i);
      const auto dir = dst-lpos;
      shadow.setdir(dir, curr);
      occluded.mapping[i] = curr;
      occluded.occluded[curr] = 0;
      occluded.t[curr++] = 1.f;
    } else
      occluded.mapping[i] = -1;
  }
  shadow.sharedorg = lpos;
  shadow.raynum = curr;
  shadow.flags = raypacket::SHAREDORG;
}

void clearpackethit(packethit &hit) {
  loopi(MAXRAYNUM) {
    hit.id[i] = ~0x0u;
    hit.t[i] = FLT_MAX;
  }
}

void primarypoint(const raypacket &RESTRICT p,
                  const packethit &RESTRICT hit,
                  array3f &RESTRICT pos,
                  array3f &RESTRICT nor,
                  arrayi &RESTRICT mask)
{
  loopi(p.raynum) {
    if (u32(hit.id[i]) == ~0x0u)
     continue;
    const auto t = hit.t[i];
    const auto dir = get(p.vdir,i);
    const auto unormal = get(hit.n,i);
    const auto org = p.sharedorg;
    const auto normal = normalize(unormal);
    const auto position = org + t*dir + SHADOWRAYBIAS*normal;
    mask[i] = ~0x0u;
    set(nor, normal, i);
    set(pos, position, i);
  }
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
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; ++y) {
    const auto yoffset = screensize.x*y;
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; ++x, ++idx) {
      const auto offset = x+yoffset;
      if (hit.ishit(idx)) {
        const auto n = clamp(normalize(hit.getnormal(idx)), vec3f(zero), vec3f(one));
        const auto rgb = vec3i(n*255.f);
        pixels[offset] = rgb.x|(rgb.y<<8)|(rgb.z<<16)|(0xff<<24);
      } else
        pixels[offset] = 0;
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
  int idx = 0;
  for (auto y = tileorg.y; y < tileorg.y+TILESIZE; ++y) {
    const auto yoffset = screensize.x*y;
    for (auto x = tileorg.x; x < tileorg.x+TILESIZE; ++x, ++idx) {
      const auto offset = x+yoffset;
      const auto remapped = occluded.mapping[idx];
      if (remapped != -1 && !occluded.occluded[remapped]) {
        const auto n = get(nor, idx);
        const auto l = normalize(shadow.dir(remapped));
        const auto shade = -dot(n, l);
        const auto d = int(255.f*min(max(0.f,shade),1.f));
        pixels[offset] = d|(d<<8)|(d<<16)|(0xff<<24);
      } else
        pixels[offset] = 0;
    }
  }
}
} /* namespace rt */
} /* namespace q */

