/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csgscalar.hpp -> exposes csg evaluation in plain C (no simd)
 -------------------------------------------------------------------------*/
#pragma once
#include "csg.hpp"

namespace q {
namespace csg {
// single point csg evaluation
float dist(const node*, const vec3f&, const aabb &box = aabb::all());

INLINE void set(array3f &v, vec3f u, u32 idx) {
  v[0][idx]=u.x; v[1][idx]=u.y; v[2][idx]=u.z;
}
INLINE vec3f get(const array3f &v, u32 idx) {
  return vec3f(v[0][idx], v[1][idx], v[2][idx]);
}

// many points csg evaluation
#include "csgdecl.hxx"
} /* namespace csg */
} /* namespace q */

