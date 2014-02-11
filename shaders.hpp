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
struct fragdatadesc {
  typedef vector<fragdatadesc> vec;
  INLINE fragdatadesc() {}
  fragdatadesc(vec **v, u32 loc, const char *name, const char *type);
  const char *name, *type;
  u32 loc;
};
struct attribdesc {
  typedef vector<attribdesc> vec;
  INLINE attribdesc() {}
  attribdesc(vec **v, u32 loc, const char *name, const char *type);
  const char *name, *type;
  u32 loc;
};
struct uniformdesc {
  typedef vector<uniformdesc> vec;
  INLINE uniformdesc() {}
  uniformdesc(vec **v, u32 &loc, const char *name, const char *type,
              bool vertex, int defaultvalue=0, bool hasdefault=false);
  u32 *loc;
  const char *name, *type;
  int defaultvalue;
  bool hasdefault;
  bool vertex;
};
struct includedesc {
  typedef vector<includedesc> vec;
  INLINE includedesc() {}
  includedesc(vec **v, const char *source, bool vertex);
  const char *source;
  bool vertex;
};
typedef void (*rulescallback)(ogl::shaderrules&, ogl::shaderrules&);

struct shaderdesc {
  const char *vppath, *fppath, *vp, *fp;
  vector<uniformdesc> **uniform;
  vector<attribdesc> **attrib;
  vector<fragdatadesc> **fragdata;
  vector<includedesc> **include;
  rulescallback rulescb;
};

typedef void (*destroycallback)();
struct destroyregister {
  destroyregister(destroycallback cb);
};
struct shaderregister {
  shaderregister(ogl::shadertype &s, const shaderdesc &r, const char *name);
};

/*-------------------------------------------------------------------------
 - shader builder for shaders specified using the above macro system
 -------------------------------------------------------------------------*/
struct builder : ogl::shaderbuilder {
  builder(const shaderdesc &desc);
  void setrules(ogl::shaderrules &vert, ogl::shaderrules &frag);
  void setuniform(ogl::shadertype&);
  void setattrib(ogl::shadertype&);
  void setfragdata(ogl::shadertype&);
  const shaderdesc &desc;
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

