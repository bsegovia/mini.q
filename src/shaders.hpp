/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> shader system mostly based on c++ premain and macros
 -------------------------------------------------------------------------*/
#pragma once
#include "ogl.hpp"
#include "shaders.hxx"
#include "base/stl.hpp"
#include "base/sys.hpp"

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
  uniformdesc(vec **v, u32 offset, const char *name, const char *type,
              bool vertex, const char *arraysize=NULL,
              int defaultvalue=0, bool hasdefault=false);
  u32 offset;
  const char *name, *type, *arraysize;
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
typedef void (*rulescallback)(ogl::shaderrules&, ogl::shaderrules&, u32 rule);

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
  shaderregister(ogl::shadertype &s, const shaderdesc &r, const char *name, u32 sz, u32);
  shaderregister(void *s, const shaderdesc &r, const char *name, u32 sz, u32 num=1);
};

/*-------------------------------------------------------------------------
 - shader builder for shaders specified using the above macro system
 -------------------------------------------------------------------------*/
struct builder : ogl::shaderbuilder {
  builder(const shaderdesc &desc, u32 rule = 0);
  void setrules(ogl::shaderrules &vert, ogl::shaderrules &frag);
  void setuniform(ogl::shadertype&);
  void setattrib(ogl::shadertype&);
  void setfragdata(ogl::shadertype&);
  const shaderdesc &desc;
  u32 rule;
};

/*-------------------------------------------------------------------------
 - declare the source code for all game shaders and shader libraries
 -------------------------------------------------------------------------*/
void start();
void finish();
} /* namespace shaders */
} /* namespace q */

