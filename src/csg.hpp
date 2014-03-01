/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace csg {

/*--------------------------------------------------------------------------
 - simple material system. each material has a type and an index
 -------------------------------------------------------------------------*/
enum mattype : u32 {
  MAT_AIR = 0,
  MAT_SNOISE,
  MAT_GRID
};

static const u32 MAT_AIR_INDEX = 0;
struct material {
  INLINE material(mattype type = MAT_AIR) : type(type) {}
  mattype type;
};

material *getmaterial(u32 matindex);

/*--------------------------------------------------------------------------
 - all basic CSG nodes
 -------------------------------------------------------------------------*/
enum CSGOP {
  C_UNION, C_DIFFERENCE, C_INTERSECTION,
  C_SPHERE, C_BOX, C_PLANE, C_CYLINDERXZ, C_CYLINDERYZ, C_CYLINDERXY,
  C_TRANSLATION, C_ROTATION,
  C_INVALID = 0xffffffff
};
struct node {
  INLINE node(CSGOP type, const aabb &box = aabb::empty()) :
    box(box), type(type) {}
  virtual ~node() {}
  aabb box;
  CSGOP type;
};

node *makescene();
void destroyscene(node *n);

float dist(const node*, const vec3f&, const aabb &box = aabb::all());
void dist(const node*, const vec3f*, float *d, u32 *mat, int num, const aabb &box);
} /* namespace csg */
} /* namespace q */

