/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#pragma once
#include "base/ref.hpp"
#include "base/math.hpp"
#include "soa.hpp"

namespace q {
namespace csg {

/*--------------------------------------------------------------------------
 - simple material system. each material has a type and an index
 -------------------------------------------------------------------------*/
enum mattype : u32 {
  MAT_AIR = 0,
  MAT_SNOISE,
  MAT_SIMPLE
};

// init the csg module
void start(void);
// stop the csg module
void finish(void);

static const u32 MAT_AIR_INDEX = 0;
static const u32 MAT_SNOISE_INDEX = 1;
static const u32 MAT_SIMPLE_INDEX = 2;
struct material {
  INLINE material(mattype type = MAT_AIR) : type(type) {}
  mattype type;
};
material *getmaterial(u32 matindex);

/*--------------------------------------------------------------------------
 - all basic CSG nodes
 -------------------------------------------------------------------------*/
enum CSGOP {
  C_EMPTY, C_UNION, C_DIFFERENCE, C_INTERSECTION, C_REPLACE,
  C_SPHERE, C_BOX, C_PLANE, C_CYLINDERXZ, C_CYLINDERYZ, C_CYLINDERXY,
  C_TRANSLATION, C_ROTATION,
  C_INVALID = 0xffffffff
};
struct node : refcount {
  INLINE node(CSGOP type, const aabb &box = aabb::empty()) :
    box(box), type(type) {}
  virtual ~node() {}
  INLINE void setmin(float x, float y, float z) {box.pmin=vec3f(x,y,z);}
  INLINE void setmax(float x, float y, float z) {box.pmax=vec3f(x,y,z);}
  INLINE aabb getbox() const { return box; }
  aabb box;
  CSGOP type;
};

node *makescene();
void destroyscene(node *n);

/*--------------------------------------------------------------------------
 - for soa computations
 -------------------------------------------------------------------------*/
static const u32 MAXPOINTNUM = 64u;
typedef CACHE_LINE_ALIGNED q::arrayi<MAXPOINTNUM> arrayi;
typedef CACHE_LINE_ALIGNED q::arrayf<MAXPOINTNUM> arrayf;
typedef CACHE_LINE_ALIGNED q::array2f<MAXPOINTNUM> array2f;
typedef CACHE_LINE_ALIGNED q::array3f<MAXPOINTNUM> array3f;
typedef CACHE_LINE_ALIGNED q::array4f<MAXPOINTNUM> array4f;
} /* namespace csg */
} /* namespace q */

