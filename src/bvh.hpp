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

struct newpackethit : noncopyable {
  array<float,raypacket::MAXRAYNUM> t;
  array<float,raypacket::MAXRAYNUM> u;
  array<float,raypacket::MAXRAYNUM> v;
  array<float,raypacket::MAXRAYNUM> nn[3];
  array<u32,raypacket::MAXRAYNUM> id;
  INLINE bool ishit(u32 idx) { return id[idx] != ~0x0u; }
  INLINE vec3f n(u32 idx) { return vec3f(nn[0][idx],nn[1][idx],nn[2][idx]); }
};

// ray tracing routines (visiblity and shadow rays)
void closest(const struct intersector&, const struct ray&, hit&);
bool occluded(const struct intersector&, const struct ray&);
void closest(const struct intersector&, const struct raypacket&, newpackethit&);
void occluded(const intersector &bvhtree, const raypacket &p, newpackethit &hit);

// opaque intersector data structure
struct intersector *create(const struct primitive*, int n);
void destroy(struct intersector*);
aabb getaabb(const struct intersector*);

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

