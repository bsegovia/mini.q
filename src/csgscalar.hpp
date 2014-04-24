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

// many points csg evaluation
#include "csgdecl.hxx"
} /* namespace csg */
} /* namespace q */

