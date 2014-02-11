/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> shader system mostly based on c++ premain and macros
 -------------------------------------------------------------------------*/
#pragma once
#include "ogl.hpp"
#include "stl.hpp"
#include "sys.hpp"

namespace q {
namespace shaders {

/*-------------------------------------------------------------------------
 - boiler plate to declare a shader and interoperate with GLSL side
 -------------------------------------------------------------------------*/
struct inoutloc {
  INLINE inoutloc() {}
  inoutloc(vector<inoutloc> **appendhere,
           u32 loc, const char *name, const char *type,
           bool attrib);
  const char *name, *type;
  u32 loc;
  bool attrib;
};
struct uniformloc {
  INLINE uniformloc() {}
  uniformloc(vector<uniformloc> **appendhere,
             u32 &loc, const char *name, const char *type,
             bool vertex, int defaultvalue=0, bool hasdefault=false);
  u32 *loc;
  const char *name, *type;
  int defaultvalue;
  bool hasdefault;
  bool vertex;
};
struct includedirective {
  INLINE includedirective() {}
  includedirective(vector<includedirective> **appendhere, const char *source, bool vertex);
  const char *source;
  bool vertex;
};
typedef void (*rulescallback)(ogl::shaderrules&, ogl::shaderrules&);

struct shaderresource {
  const char *vppath, *fppath, *vp, *fp;
  vector<uniformloc> **uniform;
  vector<inoutloc> **attrib, **fragdata;
  vector<includedirective> **include;
  rulescallback rulescb;
};

typedef void (*destroycallback)();
struct destroyregister {
  destroyregister(destroycallback cb);
};
struct shaderregister {
  shaderregister(ogl::shadertype &s, const shaderresource &r, const char *name);
};

/*-------------------------------------------------------------------------
 - shader builder for shaders specified using the above macro system
 -------------------------------------------------------------------------*/
struct builder : ogl::shaderbuilder {
  builder(const shaderresource &rsc);
  void setrules(ogl::shaderrules &vert, ogl::shaderrules &frag);
  void setuniform(ogl::shadertype&);
  void setinout(ogl::shadertype&);
  const shaderresource &rsc;
};

/*-------------------------------------------------------------------------
 - declare the source code for all game shaders and shader libraries
 -------------------------------------------------------------------------*/
#define SHADER(NAME) extern const char NAME##_vp[], NAME##_fp[];
SHADER(fxaa);
SHADER(fixed);
SHADER(forward);
SHADER(deferred);
SHADER(shadertoy);
SHADER(debugunsplit);
SHADER(simple_material);
SHADER(split_deferred);
#undef SHADER

extern const char noise2D[], noise3D[], noise4D[], fxaa[], hell[], sky[];
extern const char font_fp[];
extern const char dfrm_fp[];
void start();
void finish();
} /* namespace shaders */
} /* namespace q */

