/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> implements the shader system
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {

template <typename T> INLINE void allocateappend(vector<T> **x, T *elem) {
  if (*x == NULL) *x = NEWE(vector<T>);
  (*x)->add(*elem);
}

fragdatadesc::fragdatadesc(fragdatadesc::vec **v, u32 loc, const char *name, const char *type)
  : name(name), type(type), loc(loc) {allocateappend(v, this);}

attribdesc::attribdesc(attribdesc::vec **v, u32 loc, const char *name, const char *type)
  : name(name), type(type), loc(loc) {allocateappend(v, this);}

includedesc::includedesc(includedesc::vec **v, const char *source, bool vertex)
  : source(source), vertex(vertex) {allocateappend(v, this);}

uniformdesc::uniformdesc(uniformdesc::vec **v, u32 offset,
                         const char *name, const char *type,
                         bool vertex, const char *arraysize,
                         int defaultvalue, bool hasdefault)
  : offset(offset), name(name), type(type), arraysize(arraysize),
    defaultvalue(defaultvalue), hasdefault(hasdefault),
    vertex(vertex)
{allocateappend(v, this);}

destroyregister::destroyregister(destroycallback cb) {atexit(cb);}

struct shaderdef {
  ogl::shadertype *s;
  const shaderdesc *d;
  const char *n;
  u32 idx;
};
static vector<shaderdef> *allshaders = NULL;

shaderregister::shaderregister(ogl::shadertype &s, const shaderdesc &desc, const char *name, u32) {
  if (allshaders == NULL) allshaders = NEWE(vector<shaderdef>);
  allshaders->add({&s, &desc, name, 0u});
}

shaderregister::shaderregister(ogl::shadertype *s, const shaderdesc &desc, const char *name, u32 num) {
  if (allshaders == NULL) allshaders = NEWE(vector<shaderdef>);
  loopi(int(num)) allshaders->add({s+i, &desc, name, u32(i)});
}

void start() {
  if (allshaders) loopv(*allshaders) {
    const auto &def = (*allshaders)[i];
    const auto b = NEW(builder, *def.d, def.idx);
    if (!b->build(*def.s, ogl::loadfromfile()))
      ogl::shadererror(true, def.n);
  }
}
void finish() {
  if (allshaders) loopv(*allshaders) ogl::destroyshader(*(*allshaders)[i].s);
  SAFE_DEL(allshaders);
}

builder::builder(const shaderdesc &desc, u32 rule) :
  ogl::shaderbuilder(desc.vppath, desc.fppath, desc.vp, desc.fp),
  desc(desc), rule(rule)
{}
void builder::setrules(ogl::shaderrules &vertrules, ogl::shaderrules &fragrules) {
  if (*desc.uniform) {
    auto &uni = **desc.uniform;
    loopv(uni) {
      string rule;
      if (uni[i].arraysize == NULL)
        sprintf_s(rule)("uniform %s %s;\n", uni[i].type, uni[i].name);
      else
        sprintf_s(rule)("uniform %s %s[%s];\n", uni[i].type, uni[i].name, uni[i].arraysize);
      if (uni[i].vertex)
        vertrules.add(NEWSTRING(rule));
      else
        fragrules.add(NEWSTRING(rule));
    }
  }
  if (*desc.attrib) {
    auto &att = **desc.attrib;
    loopv(att) {
      sprintf_sd(rule)("VS_IN %s %s;\n", att[i].type, att[i].name);
      vertrules.add(NEWSTRING(rule));
    }
  }
#if !defined(__WEBGL__)
  if (*desc.fragdata) {
    auto &fd = **desc.fragdata;
    loopv(fd) {
      sprintf_sd(rule)("out %s %s;\n", fd[i].type, fd[i].name);
      fragrules.add(NEWSTRING(rule));
    }
  }
#endif
  if (*desc.include) {
    auto &inc = **desc.include;
    loopv(inc) fragrules.add(NEWSTRING(inc[i].source));
  }
  desc.rulescb(vertrules, fragrules, rule);
}
void builder::setuniform(ogl::shadertype &s) {
  if (*desc.uniform) {
    auto &uniform = **desc.uniform;
    loopv(uniform) {
      const auto idx = (uniform[i].offset-sizeof(ogl::shadertype))/sizeof(u32);
      OGLR(s.internal[idx], GetUniformLocation, s.program, uniform[i].name);
      if (uniform[i].hasdefault)
        OGL(Uniform1i, s.internal[idx], uniform[i].defaultvalue);
    }
  }
}
void builder::setattrib(ogl::shadertype &s) {
  if (*desc.attrib) {
    auto &attrib = **desc.attrib;
    loopv(attrib)
      OGL(BindAttribLocation, s.program, attrib[i].loc, attrib[i].name);
  }
}
void builder::setfragdata(ogl::shadertype &s) {
#if !defined(__WEBGL__)
  if (*desc.fragdata) {
    auto &fragdata = **desc.fragdata;
    loopv(fragdata)
      OGL(BindFragDataLocation, s.program, fragdata[i].loc, fragdata[i].name);
  }
#endif
}
#include "shaders.cxx"
} /* namespace q */
} /* namespace shaders */

