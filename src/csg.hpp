/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace csg {
struct node *makescene();
void destroyscene(struct node *n);

// distance, material, required tesselation size
struct point {
  INLINE point(float d, float s=FLT_MAX, u32 m=0) : dist(d), size(s), mat(m) {}
  INLINE point() {}
  float dist;
  float size;
  u32 mat;
};

point dist(const node &n, const vec3f &pos, const aabb &box = aabb::all());
void dist(const node &n, const vec3f *pos, float *d, int num, const aabb &box);
} /* namespace csg */
} /* namespace q */

