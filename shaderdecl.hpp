/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaderdecl.hpp -> macro hell for glsl <-> c++ interoperability
 -------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 - we define our macro system here...
 -------------------------------------------------------------------------*/
#if !defined(__WEBGL__)
#define LOCATIONS\
  static vector<shaders::inoutloc> *attrib = NULL, *fragdata = NULL;\
  static vector<shaders::uniformloc> *uniform = NULL;\
  static vector<shaders::includedirective> *include = NULL;
#define FRAGDATA(T,N,LOC)\
  static shaders::inoutloc N##loc(&fragdata,LOC,#N,#T,false);
#else
#define LOCATIONS\
  static vector<shaders::inoutloc> *attrib = NULL;\
  static vector<shaders::uniformloc> *uniform = NULL;\
  static vector<shaders::includedirective> *include = NULL;
#define FRAGDATA(T,N,LOC)
#endif
#define PUNIFORMI(T,N,X,V)\
  u32 N; static const shaders::uniformloc N##loc(&uniform,N,#N,#T,V,X,true);
#define PUNIFORM(T,N,V)\
  u32 N; static const shaders::uniformloc N##loc(&uniform,N,#N,#T,V);
#define PINCLUDE(N,V)\
  static const shaders::includedirective N##include##V(&include,shaders::N,V);
#define VATTRIB(T,N,LOC)\
  static const shaders::inoutloc N##loc(&attrib,LOC,#N,#T,true);

#define BEGIN_SHADER(SHADER, NAMESPACE) namespace NAMESPACE {\
  static ogl::shadertype shader;\
  static const char vppath[] = "data/shaders/" STRINGIFY(SHADER) "_vp.glsl";\
  static const char fppath[] = "data/shaders/" STRINGIFY(SHADER) "_fp.glsl";\
  static const char *vp = shaders:: JOIN(SHADER,_vp);\
  static const char *fp = shaders:: JOIN(SHADER,_fp);\
  LOCATIONS

#if defined(RELEASE)
#define DESTROY_SHADER
#else
#define DESTROY_SHADER\
  static void destroy() {\
    SAFE_DEL(attrib);\
    SAFE_DEL(fragdata);\
    SAFE_DEL(uniform);\
    SAFE_DEL(include);\
  }\
  static const shaders::destroyregister destroyreg(destroy);
#endif

#define END_SHADER(N)\
  static void destroy() {\
    SAFE_DEL(attrib);\
    SAFE_DEL(fragdata);\
    SAFE_DEL(uniform);\
    SAFE_DEL(include);\
  }\
  static const shaders::destroyregister destroyreg(destroy);\
  static const shaders::shaderresource rsc = {\
    vppath, fppath, vp, fp, &uniform, &attrib, &fragdata, &include, RULES\
  };\
  static const shaders::shaderregister shaderreg(shader,rsc,STRINGIFY(N));\
}
#ifndef SHADERNAMESPACE
#define SHADERNAMESPACE SHADERNAME
#endif

/*-------------------------------------------------------------------------
 - ... and we instantiate the shader descriptor here
 -------------------------------------------------------------------------*/
BEGIN_SHADER(SHADERNAME, SHADERNAMESPACE)

#define INCLUDE(N) PINCLUDE(N,true)
#define UNIFORM(T,N) PUNIFORM(T,N,true)
#define UNIFORMI(T,N,X) PUNIFORMI(T,N,X,true)
#include VERTEX_PROGRAM
#undef INCLUDE
#undef UNIFORM
#undef UNIFORMI

#define INCLUDE(N) PINCLUDE(N,false)
#define UNIFORM(T,N) PUNIFORM(T,N,false)
#define UNIFORMI(T,N,X) PUNIFORMI(T,N,X,false)
#include FRAGMENT_PROGRAM
#undef INCLUDE
#undef UNIFORM
#undef UNIFORMI

END_SHADER(SHADERNAME)
#undef SHADERNAMESPACE
#undef SHADERNAME
#undef VERTEX_PROGRAM
#undef FRAGMENT_PROGRAM
#undef LOCATIONS
#undef FRAGDATA
#undef PUNIFORMI
#undef PUNIFORM
#undef PINCLUDE
#undef VATTRIB
#undef BEGIN_SHADER
#undef DESTROY_SHADER
#undef END_SHADER

