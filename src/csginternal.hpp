/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csginternal.hpp -> exposes shared structures and routines for csg
 -------------------------------------------------------------------------*/
#pragma once
#include "csg.hpp"

namespace q {
namespace csg {
struct emptynode : node {
  INLINE emptynode() : node(C_EMPTY) {}
};
// we need to be careful with null nods
static INLINE aabb fixedaabb(const ref<node> &n) {
  return n.ptr?n->box:aabb::empty();
}
static INLINE ref<node> fixednode(const ref<node> &n) {
  return NULL!=n.ptr?n:NEWE(emptynode);
}

#define BINARY(NAME,TYPE,C_BOX) \
struct NAME : node { \
  INLINE NAME(const ref<node> &nleft, const ref<node> &nright) :\
    node(TYPE, C_BOX), left(fixednode(nleft)), right(fixednode(nright)) {}\
  ref<node> left, right; \
};
BINARY(U, C_UNION, sum(fixedaabb(nleft), fixedaabb(nright)))
BINARY(D, C_DIFFERENCE, fixedaabb(nleft))
BINARY(I, C_INTERSECTION, intersection(fixedaabb(nleft), fixedaabb(nright)))
BINARY(R, C_REPLACE, fixedaabb(nleft));
#undef BINARY

struct materialnode : node {
  INLINE materialnode(CSGOP type, const aabb &box = aabb::empty(), u32 matindex = MAT_SIMPLE_INDEX) :
    node(type, box), matindex(matindex) {}
  u32 matindex;
};

struct box : materialnode {
  INLINE box(const vec3f &extent, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_BOX, aabb(-extent,+extent), matindex), extent(extent) {}
  INLINE box(float x, float y, float z, u32 matindex = MAT_SIMPLE_INDEX) :
    box(vec3f(x,y,z), matindex) {}
  vec3f extent;
};
struct plane : materialnode {
  INLINE plane(const vec4f &p, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_PLANE, aabb::all(), matindex), p(p) {}
  INLINE plane(float a, float b, float c, float d, u32 matindex = MAT_SIMPLE_INDEX) :
    plane(vec4f(a,b,c,d), matindex) {}
  vec4f p;
};
struct sphere : materialnode {
  INLINE sphere(float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_SPHERE, aabb(-r, r), matindex), r(r) {}
  float r;
};

INLINE aabb cyzbox(const vec2f &cyz, float r) {
  return aabb(vec3f(-FLT_MAX,-r+cyz.x,-r+cyz.y),
              vec3f(+FLT_MAX,+r+cyz.x,+r+cyz.y));;
}
INLINE aabb cxzbox(const vec2f &cxz, float r) {
  return aabb(vec3f(-r+cxz.x,-FLT_MAX,-r+cxz.y),
              vec3f(+r+cxz.x,+FLT_MAX,+r+cxz.y));
}
INLINE aabb cxybox(const vec2f &cxy, float r) {
  return aabb(vec3f(-r+cxy.x,-r+cxy.y,-FLT_MAX),
              vec3f(+r+cxy.x,+r+cxy.y,+FLT_MAX));
}

struct cylinderxz : materialnode {
  INLINE cylinderxz(const vec2f &cxz, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERXZ, cxzbox(cxz,r), matindex), cxz(cxz), r(r) {}
  INLINE cylinderxz(float x, float z, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderxz(vec2f(x,z), r, matindex) {}
  vec2f cxz;
  float r;
};
struct cylinderxy : materialnode {
  INLINE cylinderxy(const vec2f &cxy, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERXY, cxybox(cxy,r), matindex), cxy(cxy), r(r) {}
  INLINE cylinderxy(float x, float y, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderxy(vec2f(x,y), r, matindex) {}
  vec2f cxy;
  float r;
};
struct cylinderyz : materialnode {
  INLINE cylinderyz(const vec2f &cyz, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERYZ, cyzbox(cyz,r), matindex), cyz(cyz), r(r) {}
  INLINE cylinderyz(float y, float z, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderyz(vec2f(y,z), r, matindex) {}
  vec2f cyz;
  float r;
};
struct translation : node {
  INLINE translation(const vec3f &p, const ref<node> &n) :
    node(C_TRANSLATION, aabb(p+fixedaabb(n).pmin, p+fixedaabb(n).pmax)), p(p),
    n(fixednode(n)) {}
  INLINE translation(float x, float y, float z, const ref<node> &n) :
    translation(vec3f(x,y,z), n) {}
  vec3f p;
  ref<node> n;
};
struct rotation : node {
  INLINE rotation(const quat3f &q, const ref<node> &n) :
    node(C_ROTATION, aabb::all()), q(q),
    n(fixednode(n)) {}
  INLINE rotation(float deg0, float deg1, float deg2, const ref<node> &n) :
    rotation(quat3f(deg2rad(deg0),deg2rad(deg1),deg2rad(deg2)),n) {}
  quat3f q;
  ref<node> n;
};

INLINE vec3f get(const array3f &v, u32 idx) {
  return vec3f(v[0][idx], v[1][idx], v[2][idx]);
}
} /* namespace csg */
} /* namespace q */

