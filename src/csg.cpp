/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg init/function function and script interface
 -------------------------------------------------------------------------*/
#include "csg.hpp"
#include "csginternal.hpp"
#include "base/math.hpp"
#include "base/script.hpp"
#include "base/sys.hpp"

namespace q {
namespace csg {
static ref<node> root;
static void setroot(const ref<node> &node) {root = node;}
node *makescene() {
  return root.ptr;
}

void destroyscene(node *n) {
  assert(n == root.ptr);
  root = NULL;
}

void start() {
#define ENUM(NAMESPACE,NAME,VALUE)\
  static const u32 NAME = VALUE;\
  luabridge::getGlobalNamespace(script::luastate())\
  .beginNamespace(#NAMESPACE)\
    .addVariable(#NAME, const_cast<u32*>(&NAME), false)\
  .endNamespace()
  ENUM(csg, mat_air_index, MAT_AIR_INDEX);
  ENUM(csg, mat_simple_index, MAT_SIMPLE_INDEX);
  ENUM(csg, mat_snoise_index, MAT_SNOISE_INDEX);
#undef ENUM

#define ADDCLASS(NAME, CONSTRUCTOR) \
  .deriveClass<NAME,node>(#NAME)\
    .addConstructor<CONSTRUCTOR, ref<NAME>>()\
  .endClass()
  luabridge::getGlobalNamespace(script::luastate())
  .beginNamespace("csg")
    .addFunction("setroot", setroot)
    .beginClass<node>("node")
      .addFunction("setmin", &node::setmin)
      .addFunction("setmax", &node::setmax)
    .endClass()
    ADDCLASS(U,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(D,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(I,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(R,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(box,void(*)(float,float,float,u32))
    ADDCLASS(plane,void(*)(float,float,float,float,u32))
    ADDCLASS(sphere,void(*)(float,u32))
    ADDCLASS(cylinderxz,void(*)(float,float,float,u32))
    ADDCLASS(cylinderxy,void(*)(float,float,float,u32))
    ADDCLASS(cylinderyz,void(*)(float,float,float,u32))
    ADDCLASS(translation,void(*)(float,float,float,const ref<node>&))
    ADDCLASS(rotation,void(*)(float,float,float,const ref<node>&))
  .endNamespace();
#undef ADDCLASS
}
void finish() {root=NULL;}
} /* namespace csg */
} /* namespace q */

