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

enum {
  MAT_AIR,
  MAT_SNOISE,
  MAT_GRID
};

struct material {
  INLINE material(u32 type = MAT_AIR) : type(type) {}
  u32 type;
};

// These are just pre-built material to test code
extern const material airmat, snoisemat, gridmat;

// distance, material, required tesselation size
struct point {
  INLINE point(float d, float s=FLT_MAX, const material *mat = &airmat) :
    dist(d), size(s), mat(mat) {}
  INLINE point() {}
  float dist, size;
  const material *mat;
};

point dist(const node *n, const vec3f &pos, const aabb &box = aabb::all());
void dist(const node *n, const vec3f *pos, float *d, int num, const aabb &box);
} /* namespace csg */
} /* namespace q */

