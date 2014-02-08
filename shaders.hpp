/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> shader system mostly based on c++ premain and macros
 -------------------------------------------------------------------------*/
#include "ogl.hpp"
#include "stl.hpp"
#include "sys.hpp"

namespace q {
namespace shaders {

#if !defined(__WEBGL__)
#define LOCATIONS\
  static vector<shaders::shaderlocation> *attrib = NULL, *fragdata = NULL;\
  static vector<shaders::uniformlocation> *uniform = NULL;
#define FRAGDATA(T,N,LOC)\
  static shaders::shaderlocation N##loc(&fragdata,LOC,#N,#T,false);
#else
#define LOCATIONS\
  static vector<shaders::shaderlocation> *attrib = NULL;\
  static vector<shaders::uniformlocation> *uniform = NULL;
#define FRAGDATA(T,N,LOC)
#endif
#define UNIFORMI(T,N,X,V)\
  u32 N; static const shaders::uniformlocation N##loc(&uniform,N,#N,#T,V,X,true);
#define UNIFORM(T,N,V)\
  u32 N; static const shaders::uniformlocation N##loc(&uniform,N,#N,#T,V);
#define VATTRIB(T,N,LOC)\
  u32 N; static const shaders::shaderlocation N##loc(&attrib,LOC,#N,#T,true);

#define VUNIFORM(T,N) UNIFORM(T,N,true)
#define VUNIFORMI(T,N,X) UNIFORMI(T,N,X,true)
#define FUNIFORM(T,N) UNIFORM(T,N,false)
#define FUNIFORMI(T,N,X) UNIFORMI(T,N,X,false)

#define BEGIN_SHADER(N) namespace N {\
  static ogl::shadertype shader;\
  static const char vppath[] = "data/shaders/" #N "_vp.glsl";\
  static const char fppath[] = "data/shaders/" #N "_fp.glsl";\
  static const char *vp = shaders:: N##_vp;\
  static const char *fp = shaders:: N##_fp;\
  LOCATIONS
#define END_SHADER(N)\
  void destroy() {\
    SAFE_DEL(attrib);\
    SAFE_DEL(fragdata);\
    SAFE_DEL(uniform);\
  }\
  static const shaders::destroyregister destroyreg(destroy);\
  static const shaders::shaderresource rsc = {\
    vppath, fppath, vp, fp, &uniform, &attrib, &fragdata, rules\
  };\
}

struct shaderlocation {
  INLINE shaderlocation() {}
  shaderlocation(vector<shaderlocation> **appendhere,
                 u32 loc, const char *name, const char *type,
                 bool attrib);
  const char *name, *type;
  u32 loc;
  bool attrib;
};
struct uniformlocation {
  INLINE uniformlocation() {}
  uniformlocation(vector<uniformlocation> **appendhere,
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
  vector<uniformlocation> **uniform;
  vector<shaderlocation> **attrib;
  vector<shaderlocation> **fragdata;
  rulescallback rulescb;
};

typedef void (*destroycallback)();
struct destroyregister {
  destroyregister(destroycallback cb) {atexit(cb);}
};

struct builder : ogl::shaderbuilder {
  builder(const shaderresource &rsc);
  void setrules(ogl::shaderrules &vertrules, ogl::shaderrules &fragrules);
  void setuniform(ogl::shadertype &s);
  void setvarying(ogl::shadertype &s);
  const shaderresource &rsc;
};

#define DEBUG_UNSPLIT 1
#define SHADER(NAME)\
extern const char NAME##_vp[], NAME##_fp[];
SHADER(fixed);
SHADER(forward);
SHADER(deferred);
SHADER(debugunsplit);
#undef SHADER

extern const char noise2D[], noise3D[], noise4D[];
extern const char font_fp[];
extern const char dfrm_fp[];

} /* namespace shaders */
} /* namespace q */

