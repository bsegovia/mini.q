/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvh.hpp -> exposes bvh routines (build and traversal)
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"
#include "base/sys.hpp"

namespace q {
namespace rt {

// hit point when tracing a ray inside a bvh
struct hit {
  float t,u,v;
  vec3f n;
  u32 id;
  INLINE hit(float tmax=FLT_MAX) : t(tmax), id(~0x0u) {}
  INLINE bool is_hit(void) const { return id != ~0x0u; }
};

static const u32 MAXRAYNUM = 256u;
typedef CACHE_LINE_ALIGNED array<int,MAXRAYNUM> arrayi;
typedef CACHE_LINE_ALIGNED array<float,MAXRAYNUM> arrayf;
typedef CACHE_LINE_ALIGNED arrayf array2f[2];
typedef CACHE_LINE_ALIGNED arrayf array3f[3];
typedef CACHE_LINE_ALIGNED arrayf array4f[4];

struct CACHE_LINE_ALIGNED raypacket {
  static const u32 SHAREDORG     = 1<<0;
  static const u32 SHAREDDIR     = 1<<1;
  static const u32 INTERVALARITH = 1<<2;
  static const u32 CORNERRAYS    = 1<<3;
  INLINE raypacket(void) : raynum(0), flags(0) {}
  INLINE void setorg(vec3f org, u32 rayid) {
    vorg[0][rayid] = org.x;
    vorg[1][rayid] = org.y;
    vorg[2][rayid] = org.z;
  }
  INLINE void setdir(vec3f dir, u32 rayid) {
    vdir[0][rayid] = dir.x;
    vdir[1][rayid] = dir.y;
    vdir[2][rayid] = dir.z;
  }
  INLINE vec3f org(u32 rayid=0) const {
    return vec3f(vorg[0][rayid], vorg[1][rayid], vorg[2][rayid]);
  }
  INLINE vec3f dir(u32 rayid=0) const {
    return vec3f(vdir[0][rayid], vdir[1][rayid], vdir[2][rayid]);
  }
  array3f vorg;     // only used when SHAREDORG is *not* set
  array3f vdir;     // only used when SHAREDDIR is *not* set
  vec3f sharedorg;  // only used when SHAREDORG is set
  vec3f shareddir;  // only used when SHAREDDIR is set
  u32 raynum; // number of rays active in the packet
  u32 flags;  // exposes property of the packet
};

struct CACHE_LINE_ALIGNED packethit : noncopyable {
  arrayf t, u, v;
  array3f n;
  arrayi id;
  INLINE bool ishit(u32 idx) const { return id[idx] != -1; }
  INLINE vec3f getnormal(u32 idx) const { return vec3f(n[0][idx],n[1][idx],n[2][idx]); }
};

struct CACHE_LINE_ALIGNED packetshadow : noncopyable {
  arrayf t;
  arrayi occluded;
};
struct intersector;

// opaque intersector data structure
struct intersector *create(const struct primitive*, int n);
void destroy(intersector*);
aabb getaabb(const intersector*);

// May be either a triangle, a bounding box and an intersector
struct primitive {
  enum { TRI, INTERSECTOR };
  INLINE primitive(void) {}
  INLINE primitive(vec3f a, vec3f b, vec3f c) : isec(NULL), type(TRI) {
    v[0]=a;
    v[1]=b;
    v[2]=c;
  }
  INLINE primitive(const intersector *isec) : isec(isec), type(INTERSECTOR) {
    const aabb box = rt::getaabb(isec);
    v[0]=box.pmin;
    v[1]=box.pmax;
  }
  INLINE aabb getaabb(void) const {
    if (type == TRI)
      return aabb(min(min(v[0],v[1]),v[2]), max(max(v[0],v[1]),v[2]));
    else
      return aabb(v[0],v[1]);
  }
  const intersector *isec;
  vec3f v[3];
  u32 type;
};
} /* namespace rt */
} /* namespace q */

