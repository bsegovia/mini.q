/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaderdecl.hpp -> macro hell for glsl <-> c++ interoperability
 -------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 - first we define a shadertype with all the given uniforms
 -------------------------------------------------------------------------*/
#define SHADER(N)
#define VATTRIB(T,N,LOC)
#define FRAGDATA(T,N,LOC)
#define UNIFORM(T,N) u32 N;
#define UNIFORMI(T,N,I) u32 N;
#define UNIFORMARRAY(T,N,S) u32 N;
#define INCLUDE(N)
namespace SHADERNAME {
struct shadertype : ogl::shadertype {
#include VERTEX_PROGRAM
#include FRAGMENT_PROGRAM
};
#include "shaderundef.hxx"
template <u32 n> struct shadertypetrait    {typedef shadertype type[n];};
template <>      struct shadertypetrait<1> {typedef shadertype type;};
} /* namespace SHADERNAME */

/*-------------------------------------------------------------------------
 - we instantiate the shaderdescriptor here
 -------------------------------------------------------------------------*/
#if !defined(__WEBGL__)
#define PFRAGDATA(T,N,LOC)\
  static shaders::fragdatadesc N##desc(&fragdata,LOC,#N,#T);
#else
#define PFRAGDATA(T,N,LOC)
#endif

#define PUNIFORMI(T,N,X,V)\
  static const shaders::uniformdesc N##desc(&uniform,offsetof(shadertype,N),#N,#T,V,NULL,X,true);
#define PUNIFORM(T,N,V)\
  static const shaders::uniformdesc N##desc(&uniform,offsetof(shadertype,N),#N,#T,V);
#define PUNIFORMARRAY(T,N,V,S)\
  static const shaders::uniformdesc N##desc(&uniform,offsetof(shadertype,N),#N,#T,V,#S);
#define PINCLUDE(N,V)\
  static const shaders::includedesc N##include##V(&include,shaders::N,V);

#ifndef SHADERVARIANT
#define SHADERVARIANT 1
#endif

namespace SHADERNAME {
static shadertypetrait<SHADERVARIANT>::type s;
static vector<shaders::fragdatadesc> *fragdata = NULL;
static vector<shaders::attribdesc> *attrib = NULL;
static vector<shaders::uniformdesc> *uniform = NULL;
static vector<shaders::includedesc> *include = NULL;

#define SHADER(N)\
static const char vppath[] = "data/shaders/" STRINGIFY(N) "_vp.glsl";\
static const char *vp = shaders:: JOIN(N,_vp);
#define VATTRIB(T,N,LOC)\
  static const shaders::attribdesc N##desc(&attrib,LOC,#N,#T);
#define INCLUDE(N) PINCLUDE(N,true)
#define UNIFORM(T,N) PUNIFORM(T,N,true)
#define UNIFORMI(T,N,X) PUNIFORMI(T,N,X,true)
#define UNIFORMARRAY(T,N,S) PUNIFORMARRAY(T,N,true,S)
#include VERTEX_PROGRAM
#include "shaderundef.hxx"

#define SHADER(N)\
static const char fppath[] = "data/shaders/" STRINGIFY(N) "_fp.glsl";\
static const char *fp = shaders:: JOIN(N,_fp);
#define FRAGDATA(T,N,LOC) PFRAGDATA(T,N,LOC)
#define INCLUDE(N) PINCLUDE(N,false)
#define UNIFORM(T,N) PUNIFORM(T,N,false)
#define UNIFORMI(T,N,X) PUNIFORMI(T,N,X,false)
#define UNIFORMARRAY(T,N,S) PUNIFORMARRAY(T,N,false,S)
#include FRAGMENT_PROGRAM
#include "shaderundef.hxx"

#if !defined(RELEASE)
static void destroy() {
  SAFE_DEL(attrib);
  SAFE_DEL(fragdata);
  SAFE_DEL(uniform);
  SAFE_DEL(include);
}
static const shaders::destroyregister destroyreg(destroy);
#endif

static const shaders::shaderdesc rsc = {
  vppath, fppath, vp, fp, &uniform, &attrib, &fragdata, &include, RULES
};
static const shaders::shaderregister
  shaderreg(s, rsc, STRINGIFY(N), u32(sizeof(shadertype)), SHADERVARIANT);
} /* namespace SHADERNAME */

/*-------------------------------------------------------------------------
 - finally, we clean up this definition mess
 -------------------------------------------------------------------------*/
#undef SHADERVARIANT
#undef SHADERNAME
#undef VERTEX_PROGRAM
#undef FRAGMENT_PROGRAM
#undef PFRAGDATA
#undef PUNIFORMI
#undef PUNIFORM
#undef PUNIFORMARRAY
#undef PINCLUDE

