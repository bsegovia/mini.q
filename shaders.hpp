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
#if !defined(__WEBGL__)
#define LOCATIONS\
  static vector<shaders::inoutloc> *attrib = NULL, *fragdata = NULL;\
  static vector<shaders::uniformloc> *uniform = NULL;
#define FRAGDATA(T,N,LOC)\
  static shaders::inoutloc N##loc(&fragdata,LOC,#N,#T,false);
#else
#define LOCATIONS\
  static vector<shaders::inoutloc> *attrib = NULL;\
  static vector<shaders::uniformloc> *uniform = NULL;
#define FRAGDATA(T,N,LOC)
#endif
#define PUNIFORMI(T,N,X,V)\
  u32 N; static const shaders::uniformloc N##loc(&uniform,N,#N,#T,V,X,true);
#define PUNIFORM(T,N,V)\
  u32 N; static const shaders::uniformloc N##loc(&uniform,N,#N,#T,V);
#define VATTRIB(T,N,LOC)\
  u32 N; static const shaders::inoutloc N##loc(&attrib,LOC,#N,#T,true);

#define BEGIN_SHADER(N) namespace N {\
  static ogl::shadertype shader;\
  static const char vppath[] = "data/shaders/" #N "_vp.glsl";\
  static const char fppath[] = "data/shaders/" #N "_fp.glsl";\
  static const char *vp = shaders:: JOIN(N,_vp);\
  static const char *fp = shaders:: JOIN(N,_fp);\
  LOCATIONS

#define END_SHADER(N)\
  static void destroy() {\
    SAFE_DEL(attrib);\
    SAFE_DEL(fragdata);\
    SAFE_DEL(uniform);\
  }\
  static const shaders::destroyregister destroyreg(destroy);\
  static const shaders::shaderresource rsc = {\
    vppath, fppath, vp, fp, &uniform, &attrib, &fragdata, rules\
  };\
  static const shaders::shaderregister shaderreg(shader,rsc,#N);\
} /* namespace N */

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

typedef void (*rulescallback)(ogl::shaderrules&, ogl::shaderrules&);

struct shaderresource {
  const char *vppath, *fppath, *fp, *vp;
  vector<uniformloc> **uniform;
  vector<inoutloc> **attrib, **fragdata;
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
SHADER(fixed);
SHADER(forward);
SHADER(deferred);
SHADER(debugunsplit);
#undef SHADER

extern const char noise2D[], noise3D[], noise4D[];
extern const char font_fp[];
extern const char dfrm_fp[];
void start();
void finish();
} /* namespace shaders */
} /* namespace q */

