/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvh.hpp -> exposes bvh routines (build and traversal)
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"
#include "base/ref.hpp"
#include "base/vector.hpp"
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

struct waldtriangle {
  vec2f bn, cn, vertk, n;
  float nd;
  u32 k:2, sign:1, num:29;
  u32 id, matid;
};

struct intersector : public refcount {
  intersector(struct primitive*, int n);
  virtual ~intersector();
  INLINE aabb getaabb() const {return root[0].box;}
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
    INLINE void setptr(const T *ptr) {prim = (prim&uintptr(MASK))|uintptr(ptr);}
    INLINE u32 getoffset(void) const {return offsetflag>>SHIFT;}
    INLINE u32 getaxis(void) const {return axis;}
    INLINE u32 getflag(void) const {return offsetflag & MASK;}
    INLINE u32 isleaf(void) const {return getflag();}
    INLINE void setoffset(u32 offset) {offsetflag = (offset<<SHIFT)|getflag();}
    INLINE void setaxis(u32 d) {axis = d;}
    INLINE void setflag(u32 flag) {offsetflag = (offsetflag&~MASK)|flag;}
  };
  node *root;
  vector<waldtriangle> acc;
};
static_assert(sizeof(intersector::node) == 32,"invalid node size");

// May be either a triangle or an intersector primitive
struct primitive {
  enum { TRI, INTERSECTOR };
  INLINE primitive(void) {}
  INLINE primitive(vec3f a, vec3f b, vec3f c) : isec(NULL), type(TRI) {
    v[0]=a;
    v[1]=b;
    v[2]=c;
  }
  INLINE primitive(const ref<intersector> &isec) : isec(isec), type(INTERSECTOR) {
    const aabb box = isec->getaabb();
    v[0]=box.pmin;
    v[1]=box.pmax;
  }
  INLINE aabb getaabb(void) const {
    if (type == TRI)
      return aabb(min(min(v[0],v[1]),v[2]), max(max(v[0],v[1]),v[2]));
    else
      return aabb(v[0],v[1]);
  }
  ref<intersector> isec;
  vec3f v[3];
  u32 type;
};
} /* namespace rt */
} /* namespace q */

