/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> implements the shader system
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {

inoutloc::inoutloc(vector<inoutloc> **appendhere,
                   u32 loc, const char *name, const char *type,
                   bool attrib)
  : name(name), type(type), loc(loc), attrib(attrib)
{
  if (*appendhere == NULL) *appendhere = NEWE(vector<inoutloc>);
  (*appendhere)->add(*this);
}

uniformloc::uniformloc(vector<uniformloc> **appendhere,
                       u32 &loc, const char *name, const char *type,
                       bool vertex, int defaultvalue, bool hasdefault)
  : loc(&loc), name(name), type(type), defaultvalue(defaultvalue),
    hasdefault(hasdefault), vertex(vertex)
{
  if (*appendhere == NULL) *appendhere = NEWE(vector<uniformloc>);
  (*appendhere)->add(*this);
}

destroyregister::destroyregister(destroycallback cb) {atexit(cb);}

struct shaderdef {
  ogl::shadertype *s;
  const shaderresource *r;
  const char *name;
};
static vector<shaderdef> *allshaders = NULL;

shaderregister::shaderregister(ogl::shadertype &s, const shaderresource &rsc, const char *name) {
  if (allshaders == NULL)
    allshaders = NEWE(vector<shaderdef>);
  allshaders->add({&s, &rsc, name});
}

void start() {
  if (allshaders) loopv(*allshaders) {
    const auto b = NEW(builder, *(*allshaders)[i].r);
    if (!b->build(*(*allshaders)[i].s, ogl::loadfromfile()))
      ogl::shadererror(true, (*allshaders)[i].name);
  }
}
void finish() {SAFE_DEL(allshaders);}

builder::builder(const shaderresource &rsc) :
  ogl::shaderbuilder(rsc.vppath, rsc.fppath, rsc.vp, rsc.fp), rsc(rsc)
{}
void builder::setrules(ogl::shaderrules &vertrules, ogl::shaderrules &fragrules) {
  if (*rsc.uniform) {
    auto &uniform = **rsc.uniform;
    loopv(uniform) {
      sprintf_sd(rule)("uniform %s %s;\n", uniform[i].type, uniform[i].name);
      if (uniform[i].vertex)
        vertrules.add(NEWSTRING(rule));
      else
        fragrules.add(NEWSTRING(rule));
    }
  }
  if (*rsc.attrib) {
    auto &attrib = **rsc.attrib;
    loopv(attrib) {
      sprintf_sd(rule)("VS_IN %s %s;\n", attrib[i].type, attrib[i].name);
      vertrules.add(NEWSTRING(rule));
    }
  }
#if !defined(__WEBGL__)
  if (*rsc.fragdata) {
    auto &fragdata = **rsc.fragdata;
    loopv(fragdata) {
      sprintf_sd(rule)("out %s %s;\n", fragdata[i].type, fragdata[i].name);
      fragrules.add(NEWSTRING(rule));
    }
  }
#endif
  rsc.rulescb(vertrules, fragrules);
}
void builder::setuniform(ogl::shadertype &s) {
  if (*rsc.uniform) {
    auto &uniform = **rsc.uniform;
    loopv(uniform) {
      OGLR(*uniform[i].loc, GetUniformLocation, s.program, uniform[i].name);
      if (uniform[i].hasdefault)
        OGL(Uniform1i, *uniform[i].loc, uniform[i].defaultvalue);
    }
  }
}
void builder::setinout(ogl::shadertype &s) {
  if (*rsc.attrib) {
    auto &attrib = **rsc.attrib;
    loopv(attrib)
      OGL(BindAttribLocation, s.program, attrib[i].loc, attrib[i].name);
  }
#if !defined(__WEBGL__)
  if (*rsc.fragdata) {
    auto &fragdata = **rsc.fragdata;
    loopv(fragdata)
      OGL(BindFragDataLocation, s.program, fragdata[i].loc, fragdata[i].name);
  }
#endif
}
#include "shaders.cxx"
} /* namespace q */
} /* namespace shaders */

