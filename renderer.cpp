/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso.hpp"
#include "csg.hpp"
#include "shaders.hpp"

namespace q {
namespace rr {

/*--------------------------------------------------------------------------
 - HUD transformation and dimensions
 -------------------------------------------------------------------------*/
float VIRTH = 1.f;
vec2f scrdim() { return vec2f(float(sys::scrw), float(sys::scrh)); }
static void setscreentransform() {
  const auto scr = scrdim();
  ogl::pushmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::pushmode(ogl::PROJECTION);
  ogl::setortho(0.f, scr.x, 0.f, scr.y, -1.f, 1.f);
}
static void popscreentransform() {
  ogl::popmode(ogl::PROJECTION);
  ogl::popmode(ogl::MODELVIEW);
}

/*--------------------------------------------------------------------------
 - handle the HUD (console, scores...)
 -------------------------------------------------------------------------*/
IVAR(showstats, 0, 0, 1);
static void drawhud(int w, int h, int curfps) {
  auto cmd = con::curcmd();
  const auto scr = scrdim();
  const auto fontdim = text::fontdim();
  ogl::enablev(GL_BLEND);
  ogl::disable(GL_DEPTH_TEST);
  OGL(BlendFunc, GL_ONE, GL_ONE);
  setscreentransform();
  text::displaywidth(text::fontdim().x);
  if (cmd) text::drawf("> %s_", vec2f(8.f, scr.y-50.f), cmd);
  con::render();
  if (showstats) {
    vec2f textpos(scr.x-400.f, scr.y-50.f);
    const auto o = game::player.o;
    const auto ypr = game::player.ypr;
    text::drawf("x: %f y: %f z: %f", textpos, o.x, o.y, o.z);
    textpos.y += fontdim.y;
    text::drawf("yaw: %f pitch: %f roll: %f", textpos, ypr.x, ypr.y, ypr.z);
    textpos.y += fontdim.y;
    text::drawf("%i f/s", textpos, curfps);
  }
  popscreentransform();
  ogl::disable(GL_BLEND);
  ogl::enable(GL_DEPTH_TEST);
}

/*--------------------------------------------------------------------------
 - handle the HUD gun
 -------------------------------------------------------------------------*/
static const char *hudgunnames[] = {
  "hudguns/fist",
  "hudguns/shotg",
  "hudguns/chaing",
  "hudguns/rocket",
  "hudguns/rifle"
};
IVARP(showhudgun, 0, 1, 1);

static void drawhudmodel(int start, int end, float speed, int base) {
  md2::render(hudgunnames[game::player.gun], start, end,
              //game::player.o+vec3f(0.f,0.f,-10.f), game::player.ypr,
              game::player.o+vec3f(0.f,0.f,0.f), game::player.ypr+vec3f(0.f,0.f,0.f),
              //vec3f(zero), game::player.ypr+vec3f(90.f,0.f,0.f),
              false, 1.0f, speed, 0, base);
}

static void drawhudgun(float fovy, float aspect, float farplane) {
  if (!showhudgun) return;

#if 0
  const int rtime = game::reloadtime(game::player.gunselect);
  if (game::player.lastaction &&
      game::player.lastattackgun==game::player.gunselect &&
      game::lastmillis()-game::player.lastaction<rtime)
    drawhudmodel(7, 18, rtime/18.0f, game::player.lastaction);
  else
#endif
  drawhudmodel(6, 1, 100.f, 0);
}

/*--------------------------------------------------------------------------
 - deferred shading stuff
 -------------------------------------------------------------------------*/
struct shaderlocation {
  INLINE shaderlocation() {}
  INLINE shaderlocation(vector<shaderlocation> **appendhere, u32 loc, const char *name)
    : loc(loc), name(name)
  {
    if (*appendhere == NULL) *appendhere = NEWE(vector<shaderlocation>);
    (*appendhere)->add(*this);
  }
  u32 loc;
  const char *name;
};
struct uniformlocation {
  INLINE uniformlocation() {}
  INLINE uniformlocation(vector<uniformlocation> **appendhere, u32 &loc,
                         const char *name, int defaultvalue=0, int hasdefault=0)
    : loc(&loc), name(name), defaultvalue(defaultvalue), hasdefault(hasdefault)
  {
    if (*appendhere == NULL) *appendhere = NEWE(vector<uniformlocation>);
    (*appendhere)->add(*this);
  }
  u32 *loc;
  const char *name;
  int defaultvalue;
  int hasdefault;
};

typedef void (*rulescallback)(string &);

struct shaderresource {
  const char *vppath, *fppath, *fp, *vp;
  vector<uniformlocation> **uniform;
  vector<shaderlocation> **attrib;
  vector<shaderlocation> **fragdata;
  rulescallback rulescb;
};

typedef void (*destroycallback)();
struct destroyregister {
  destroyregister(destroycallback cb) {
    atexit(cb);
  }
};

#if !defined(__WEBGL__)
#define LOCATIONS\
  static vector<shaderlocation> *attrib = NULL;\
  static vector<shaderlocation> *fragdata = NULL;\
  static vector<uniformlocation> *uniform = NULL;
#define FRAGDATA(NAME,LOC) static shaderlocation NAME##loc(&fragdata,LOC,#NAME);
#else
#define LOCATIONS\
  static vector<shaderlocation> *attrib = NULL;\
  static vector<uniformlocation> *uniform = NULL;
#define FRAGDATA(NAME)
#endif
#define UNIFORMI(NAME,X) u32 NAME; static uniformlocation NAME##loc(&uniform,NAME,#NAME,X,1);
#define UNIFORM(NAME) u32 NAME; static uniformlocation NAME##loc(&uniform,NAME,#NAME);
#define ATTRIB(NAME,LOC) u32 NAME; static shaderlocation NAME##loc(&attrib,LOC,#NAME);

#define BEGIN_SHADER(NAME) namespace NAME {\
  static ogl::shadertype shader;\
  static const char vppath[] = "data/shaders/" #NAME "_vp.glsl";\
  static const char fppath[] = "data/shaders/" #NAME "_fp.glsl";\
  static const char *vp = shaders:: NAME##_vp;\
  static const char *fp = shaders:: NAME##_fp;\
  LOCATIONS
#define END_SHADER(NAME)\
  void destroy() {\
    SAFE_DEL(attrib);\
    SAFE_DEL(fragdata);\
    SAFE_DEL(uniform);\
  }\
  static const destroyregister destroyreg(destroy);\
  static const shaderresource rsc = {vppath,fppath,fp,vp,&uniform,&attrib,&fragdata,rules};}

#define SPLITNUM 4
static void rules(string &str) {
  sprintf_s(str)("#define SPLITNUM %f\n", float(SPLITNUM));
}

BEGIN_SHADER(deferred)
  UNIFORM(u_mvp)
  UNIFORM(u_subbufferdim)
  UNIFORM(u_rcpsubbufferdim)
  UNIFORM(u_invmvp)
  UNIFORM(u_lightpos)
  UNIFORM(u_lightpow)
  UNIFORMI(u_nortex,0)
  UNIFORMI(u_depthtex,1)
  ATTRIB(vs_pos, ogl::ATTRIB_POS0)
  FRAGDATA(rt_col, 0)
END_SHADER(deferred)

#define SHADER\
  UNIFORM(u_mvp)\
  UNIFORM(u_subbufferdim)\
  UNIFORM(u_rcpsubbufferdim)\
  UNIFORMI(u_lighttex,0)\
  UNIFORMI(u_nortex,1)\
  ATTRIB(vp_pos, ogl::ATTRIB_POS0)\
  FRAGDATA(rt_col, 0)

BEGIN_SHADER(forward)
SHADER
ATTRIB(vp_nor, ogl::ATTRIB_COL)
END_SHADER(forward)

BEGIN_SHADER(debugunsplit)
SHADER
END_SHADER(debugunsplit)

#undef SHADER

struct genericshaderbuilder : ogl::shaderbuilder {
  genericshaderbuilder(const shaderresource &rsc) :
    ogl::shaderbuilder(rsc.vppath, rsc.fppath, rsc.vp, rsc.fp), rsc(rsc)
  {}
  virtual void setrules(string &str) {
    rsc.rulescb(str);
  }
  virtual void setuniform(ogl::shadertype &s) {
    if (*rsc.uniform) {
      auto &uniform = **rsc.uniform;
      loopv(uniform) {
        OGLR(*uniform[i].loc, GetUniformLocation, s.program, uniform[i].name);
        if (uniform[i].hasdefault)
          OGL(Uniform1i, *uniform[i].loc, uniform[i].defaultvalue);
      }
    }
  }
  virtual void setvarying(ogl::shadertype &s) {
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
  const shaderresource &rsc;
};

static u32 gdethtex, gnortex, finaltex;
static u32 gbuffer, shadedbuffer;

static void initdeferred() {
  // all textures
  gnortex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  gdethtex = ogl::maketex("Tf Id Dd Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  finaltex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);

  // all frame buffer objects
  OGL(GenFramebuffers, 1, &gbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, gnortex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, gdethtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init gbuffer framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  OGL(GenFramebuffers, 1, &shadedbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, shadedbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, finaltex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init debug unsplit framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);

  // all shaders
  if (!NEW(genericshaderbuilder, deferred::rsc)->build(deferred::shader, ogl::loadfromfile()))
    ogl::shadererror(true, "deferred shader");
  if (!NEW(genericshaderbuilder, forward::rsc)->build(forward::shader, ogl::loadfromfile()))
    ogl::shadererror(true, "forward shader");
#if DEBUG_UNSPLIT
  if (!NEW(genericshaderbuilder, debugunsplit::rsc)->build(debugunsplit::shader, ogl::loadfromfile()))
    ogl::shadererror(true, "debugunsplit shader");
#endif
}

static void cleandeferred() {
  ogl::deletetextures(1, &gnortex);
  ogl::deletetextures(1, &gdethtex);
  ogl::deletetextures(1, &finaltex);
  OGL(DeleteFramebuffers, 1, &gbuffer);
  OGL(DeleteFramebuffers, 1, &shadedbuffer);
  ogl::destroyshader(deferred::shader);
  ogl::destroyshader(forward::shader);
#if DEBUG_UNSPLIT
  ogl::destroyshader(debugunsplit::shader);
#endif
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

typedef pair<vec3f,vec3f> vertex;
static u32 vertnum = 0u, indexnum = 0u;
static vector<vertex> vertices;
static vector<u32> indices;
static bool initialized_m = false;

void start() {
  initdeferred();
}
void finish() {
  vertices.destroy();
  indices.destroy();
  cleandeferred();
}
static const float CELLSIZE = 0.2f;
static void makescene() {
  if (initialized_m) return;
  const float start = sys::millis();
  const auto node = csg::makescene();
  auto m = iso::dc_mesh_mt(vec3f(zero), 2048, CELLSIZE, *node);
  loopi(m.m_vertnum) vertices.add(makepair(m.m_pos[i], m.m_nor[i]));
  loopi(m.m_indexnum) indices.add(m.m_index[i]);
  vertnum = m.m_vertnum;
  indexnum = m.m_indexnum;
  const auto duration = sys::millis() - start;
  assert(indexnum%3 == 0);
  con::out("elapsed %f ms ", duration);
  con::out("tris %i verts %i", indexnum/3, vertnum);
  initialized_m = true;
  destroyscene(node);
}
struct screenquad {
  static INLINE screenquad get() {
    const auto w = float(sys::scrw), h = float(sys::scrh);
    const screenquad qs = {w,h,w,0.f,0.f,h,0.f,0.f};
    return qs;
  };
  static INLINE screenquad getsubbuffer(vec2i xy) {
    const auto w = float(sys::scrw/SPLITNUM);
    const auto h = float(sys::scrh/SPLITNUM);
    const auto sw = w*xy.x, sh = h*xy.y;
    const auto ew = sw+w, eh = sh+h;
    const screenquad qs = {ew,eh,ew,sh,sw,eh,sw,sh};
    return qs;
  }
  float v[16];
};

// XXX just to have something to display
FVAR(lightscale, 0.f, 2000.f, 10000.f);
static const vec3f lightpos[] = {
  vec3f(8.f,20.f,8.f), vec3f(10.f,20.f,8.f),
  vec3f(12.f,20.f,8.f), vec3f(14.f,20.f,8.f),
  vec3f(6.f,20.f,8.f), vec3f(16.f,20.f,4.f),
  vec3f(16.f,21.f,7.f), vec3f(16.f,22.f,9.f),
  vec3f(8.f,20.f,9.f), vec3f(10.f,20.f,10.f),
  vec3f(12.f,20.f,12.f), vec3f(14.f,20.f,13.f),
  vec3f(6.f,20.f,14.f), vec3f(16.f,20.f,15.f),
  vec3f(15.f,21.f,7.f), vec3f(16.f,22.f,10.f)
};

static const vec3f lightpow[] = {
  vec3f(1.f,0.f,0.f), vec3f(0.f,1.f,0.f),
  vec3f(1.f,0.f,1.f), vec3f(1.f,1.f,1.f),
  vec3f(1.f,0.f,1.f), vec3f(0.f,1.f,1.f),
  vec3f(1.f,1.f,0.f), vec3f(0.f,1.f,1.f),
  vec3f(0.f,1.f,1.f), vec3f(1.f,0.f,0.f),
  vec3f(0.f,1.f,0.f), vec3f(1.f,0.f,1.f),
  vec3f(1.f,1.f,1.f), vec3f(1.f,0.f,1.f),
  vec3f(0.f,1.f,1.f), vec3f(1.f,1.f,0.f),
  vec3f(0.f,1.f,1.f), vec3f(0.f,1.f,1.f)
};

static void dodeferred(const mat4x4f &worldmvp) {
  const auto &mv = ogl::matrix(ogl::MODELVIEW);
  const auto &p  = ogl::matrix(ogl::PROJECTION);
  const auto mvp = p*mv;
  const auto invmvp = worldmvp.inverse();
  const auto w = float(sys::scrw), h = float(sys::scrh);
  const auto depthtr = invmvp * mat4x4f(vec4f(2.f/w,0.f,  0.f,0.f),
                                        vec4f(0.f,  2.f/h,0.f,0.f),
                                        vec4f(0.f,  0.f,  2.f,0.f),
                                        vec4f(-1.f,-1.f, -1.f,1.f));
  ogl::disable(GL_CULL_FACE);
  ogl::immenableflush(false);

  OGL(BindFramebuffer, GL_FRAMEBUFFER, shadedbuffer);
  const vec2f subbufferdim(float(sys::scrw/SPLITNUM), float(sys::scrh/SPLITNUM));
  const vec2f rcpsubbufferdim = rcp(subbufferdim);
  ogl::bindshader(deferred::shader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 0);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gdethtex, 1);
  OGL(UniformMatrix4fv, deferred::u_mvp, 1, GL_FALSE, &mvp.vx.x);
  OGL(UniformMatrix4fv, deferred::u_invmvp, 1, GL_FALSE, &depthtr.vx.x);
  OGL(Uniform2fv, deferred::u_subbufferdim, 1, &subbufferdim.x);
  OGL(Uniform2fv, deferred::u_rcpsubbufferdim, 1, &rcpsubbufferdim.x);


  loopi(SPLITNUM) loopj(SPLITNUM) {
    const auto idx = i+SPLITNUM*j;
    const vec3f lpow = lightscale * lightpow[idx];
    OGL(Uniform3fv, deferred::u_lightpos, 1, &lightpos[idx].x);
    OGL(Uniform3fv, deferred::u_lightpow, 1, &lpow.x);
    ogl::immdraw("Sp2", 4, screenquad::getsubbuffer(vec2i(i,j)).v);
  }
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);

#if DEBUG_UNSPLIT
  ogl::bindshader(debugunsplit::shader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, finaltex, 0);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 1);
  OGL(UniformMatrix4fv, debugunsplit::u_mvp, 1, GL_FALSE, &mvp.vx.x);
  OGL(Uniform2fv, debugunsplit::u_subbufferdim, 1, &subbufferdim.x);
  OGL(Uniform2fv, debugunsplit::u_rcpsubbufferdim, 1, &rcpsubbufferdim.x);
  ogl::immdraw("Sp2", 4, screenquad::get().v);
#endif

  ogl::immenableflush(true);
  ogl::enable(GL_CULL_FACE);
}

static void transplayer() {
  using namespace game;
  ogl::identity();
  ogl::rotate(player.ypr.z, vec3f(0.f,0.f,1.f));
  ogl::rotate(player.ypr.y, vec3f(1.f,0.f,0.f));
  ogl::rotate(player.ypr.x, vec3f(0.f,1.f,0.f));
  ogl::translate(-player.o);
}

static void setforwardmatrices(float fovy, float aspect, float farplane) {
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  ogl::matrixmode(ogl::MODELVIEW);
  transplayer();
}

static void doforward(const mat4x4f &mvp, float fovy, float aspect, float farplane) {
  setforwardmatrices(fov, aspect, farplane);
  const vec2f subbufferdim(float(sys::scrw/SPLITNUM), float(sys::scrh/SPLITNUM));
  const vec2f rcpsubbufferdim = rcp(subbufferdim);
  ogl::bindshader(forward::shader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, finaltex, 0);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 1);
  OGL(UniformMatrix4fv, forward::u_mvp, 1, GL_FALSE, &mvp.vx.x);
  OGL(Uniform2fv, forward::u_subbufferdim, 1, &subbufferdim.x);
  OGL(Uniform2fv, forward::u_rcpsubbufferdim, 1, &rcpsubbufferdim.x);
  ogl::immenableflush(false);
  ogl::immdrawelememts("Tip3c3", indexnum, &indices[0], &vertices[0].first[0]);
  ogl::immenableflush(true);
}

IVAR(linemode, 0, 0, 1);

void frame(int w, int h, int curfps) {
  const float farplane = 100.f;
  const float aspect = float(sys::scrw) / float(sys::scrh);
  const float fovy = fov / aspect;
  setforwardmatrices(fovy, aspect, farplane);

#if 0
  const float ground[] = {
    -100.f, -100.f, -100.f, 0.f, -100.f,
    -100.f, +100.f, -100.f, 0.f, +100.f,
    +100.f, -100.f, +100.f, 0.f, -100.f,
    +100.f, +100.f, +100.f, 0.f, +100.f
  };
#endif

  makescene();

  // render the scene into the frame buffer
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw("St2p3", 4, ground);
#endif
  if (vertnum != 0) {
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    ogl::bindfixedshader(ogl::FIXED_COLOR);
    ogl::immdrawelememts("Tip3c3", indexnum, &indices[0], &vertices[0].first[0]);
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  }
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  const auto &mv = ogl::matrix(ogl::MODELVIEW);
  const auto &p  = ogl::matrix(ogl::PROJECTION);
  const auto worldmvp = p*mv;

  // blit the normal buffer into screen
  setscreentransform();
  dodeferred(worldmvp);
  popscreentransform();
#if !DEBUG_UNSPLIT
  doforward(worldmvp, fovy, aspect, farplane);
#endif
  // drawhudgun(fovy, aspect, farplane);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
}
} /* namespace rr */
} /* namespace q */

