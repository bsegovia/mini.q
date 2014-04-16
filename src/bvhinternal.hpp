/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhinternal.hpp -> exposes internal structures for the BVH
 -------------------------------------------------------------------------*/
#pragma once
#include "base/vector.hpp"

namespace q {
namespace rt {

struct waldtriangle {
  vec2f bn, cn, vertk, n;
  float nd;
  u32 k:2, sign:1, num:29;
  u32 id, matid;
};

struct intersector {
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
    INLINE void setptr(const T *ptr) { prim = (prim&uintptr(MASK))|uintptr(ptr); }
    INLINE u32 getoffset(void) const { return offsetflag>>SHIFT; }
    INLINE u32 getaxis(void) const { return axis; }
    INLINE u32 getflag(void) const { return offsetflag & MASK; }
    INLINE u32 isleaf(void) const { return getflag(); }
    INLINE void setoffset(u32 offset) { offsetflag = (offset<<SHIFT)|getflag(); }
    INLINE void setaxis(u32 d) { axis = d; }
    INLINE void setflag(u32 flag) { offsetflag = (offsetflag&~MASK)|flag; }
  };
  node *root;
  vector<waldtriangle> acc;
};
static_assert(sizeof(intersector::node) == 32,"invalid node size");

INLINE aabb getaabb(const struct intersector *isec) {
  return isec->root[0].box;
}
} /* namespace rt */
} /* namespace q */

