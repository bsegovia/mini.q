/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvh.hpp -> exposes bvh routines (build and traversal)
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"
#include "base/sys.hpp"

namespace q {
namespace bvh {

// hit point when tracing a ray inside a bvh
struct hit {
  float t,u,v;
  vec3f n;
  u32 id;
  INLINE hit(float tmax=FLT_MAX) : t(tmax), id(~0x0u) {}
  INLINE bool is_hit(void) const { return id != ~0x0u; }
};
typedef CACHE_LINE_ALIGNED array<int,raypacket::MAXRAYNUM> arrayi;
typedef CACHE_LINE_ALIGNED array<float,raypacket::MAXRAYNUM> arrayf;
typedef CACHE_LINE_ALIGNED arrayf array2f[2];
typedef CACHE_LINE_ALIGNED arrayf array3f[3];
typedef CACHE_LINE_ALIGNED arrayf array4f[4];

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

// ray tracing routines (visiblity and shadow rays)
void closest(const intersector &tree, const struct ray&, hit&);
bool occluded(const intersector &tree, const struct ray&);
void closest(const intersector &tree, const raypacket &p, packethit &hit);
void occluded(const intersector &tree, const raypacket &p, packethit &hit);

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
    const aabb box = bvh::getaabb(isec);
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
} /* namespace bvh */
} /* namespace q */

