/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso.hpp"
#include "csg.hpp"
#include "shaders.hpp"

#define DEBUG_UNSPLIT 0

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
    ogl::printtimers(400.f, 400.f);
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
#define SPLITNUM 4
static void rules(ogl::shaderrules &vertrules, ogl::shaderrules &fragrules) {
  sprintf_sd(str)("#define SPLITNUM %f\n", float(SPLITNUM));
  vertrules.add(NEWSTRING(str));
  fragrules.add(NEWSTRING(str));
}

#define SHADERNAME deferred
#define VERTEX_PROGRAM "data/shaders/deferred_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/deferred_fp.decl"
#include "shaderdecl.hpp"

#define SHADERNAME forward
#define VERTEX_PROGRAM "data/shaders/forward_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/forward_fp.decl"
#include "shaderdecl.hpp"

#define SHADERNAME debugunsplit
#define VERTEX_PROGRAM "data/shaders/debugunsplit_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/debugunsplit_fp.decl"
#include "shaderdecl.hpp"

#define SHADERNAME simple_material
#define VERTEX_PROGRAM "data/shaders/simple_material_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/simple_material_fp.decl"
#include "shaderdecl.hpp"

static u32 gdethtex, gnortex, finaltex;
static u32 gbuffer, shadedbuffer;

static void initdeferred() {
  // all textures
  gnortex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  gdethtex = ogl::maketex("Tf Id Dd Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  finaltex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);

  // all frame buffer objects
  ogl::genframebuffers(1, &gbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, gnortex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, gdethtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init gbuffer framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  ogl::genframebuffers(1, &shadedbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, shadedbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, finaltex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init debug unsplit framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
}

static void cleandeferred() {
  ogl::deletetextures(1, &gnortex);
  ogl::deletetextures(1, &gdethtex);
  ogl::deletetextures(1, &finaltex);
  ogl::deleteframebuffers(1, &gbuffer);
  ogl::deleteframebuffers(1, &shadedbuffer);
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

#if 0
typedef pair<vec3f,vec3f> vertex;
static u32 vertnum = 0u, indexnum = 0u;
static vector<vertex> vertices;
static vector<u32> indices;
#endif
static u32 scenenorbo = 0u, sceneposbo = 0u, sceneibo = 0u;
static u32 indexnum = 0u;
static bool initialized_m = false;

void start() {
  initdeferred();
}
void finish() {
  ogl::deletebuffers(1, &sceneposbo);
  ogl::deletebuffers(1, &scenenorbo);
  ogl::deletebuffers(1, &sceneibo);
  cleandeferred();
}
static const float CELLSIZE = 0.2f;
static void makescene() {
  if (initialized_m) return;
  const float start = sys::millis();
  const auto node = csg::makescene();
  auto m = iso::dc_mesh_mt(vec3f(zero), 2048, CELLSIZE, *node);
  ogl::genbuffers(1, &sceneposbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, sceneposbo);
  OGL(BufferData, GL_ARRAY_BUFFER, m.m_vertnum*sizeof(vec3f), &m.m_pos[0].x, GL_STATIC_DRAW);
  ogl::genbuffers(1, &scenenorbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, scenenorbo);
  OGL(BufferData, GL_ARRAY_BUFFER, m.m_vertnum*sizeof(vec3f), &m.m_nor[0].x, GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::genbuffers(1, &sceneibo);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, sceneibo);
  OGL(BufferData, GL_ELEMENT_ARRAY_BUFFER, m.m_indexnum*sizeof(u32), &m.m_index[0], GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);
  indexnum = m.m_indexnum;

  const auto duration = sys::millis() - start;
  con::out("elapsed %f ms ", duration);
  con::out("tris %i verts %i", m.m_indexnum/3, m.m_vertnum);
  initialized_m = true;
  destroyscene(node);
}
struct screenquad {
  static INLINE screenquad get() {
    const auto w = float(sys::scrw), h = float(sys::scrh);
    const screenquad qs = {{w,h,w,0.f,0.f,h,0.f,0.f}};
    return qs;
  };
  static INLINE screenquad getsubbuffer(vec2i xy) {
    const auto w = float(sys::scrw/SPLITNUM);
    const auto h = float(sys::scrh/SPLITNUM);
    const auto sw = w*xy.x, sh = h*xy.y;
    const auto ew = sw+w, eh = sh+h;
    const screenquad qs = {{ew,eh,ew,sh,sw,eh,sw,sh}};
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

  const auto deferredtimer = ogl::begintimer("deferred", true);
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
  ogl::endtimer(deferredtimer);

#if DEBUG_UNSPLIT
  const auto unsplittimer = ogl::begintimer("unsplit", true);
  ogl::bindshader(debugunsplit::shader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, finaltex, 0);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 1);
  OGL(UniformMatrix4fv, debugunsplit::u_mvp, 1, GL_FALSE, &mvp.vx.x);
  OGL(Uniform2fv, debugunsplit::u_subbufferdim, 1, &subbufferdim.x);
  OGL(Uniform2fv, debugunsplit::u_rcpsubbufferdim, 1, &rcpsubbufferdim.x);
  ogl::immdraw("Sp2", 4, screenquad::get().v);
  ogl::endtimer(unsplittimer);
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

static void renderscene(bool flush) {
  ogl::bindbuffer(ogl::ARRAY_BUFFER, sceneposbo);
  OGL(VertexAttribPointer, ogl::ATTRIB_POS0, 3, GL_FLOAT, 0, sizeof(vec3f), NULL);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, scenenorbo);
  OGL(VertexAttribPointer, ogl::ATTRIB_COL, 3, GL_FLOAT, 0, sizeof(vec3f), NULL);
  ogl::setattribarray()(ogl::ATTRIB_POS0, ogl::ATTRIB_COL);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, sceneibo);
  if (flush) ogl::fixedflush();
  ogl::drawelements(GL_TRIANGLES, indexnum, GL_UNSIGNED_INT, NULL);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
}

static void doforward(const mat4x4f &mvp, float fovy, float aspect, float farplane) {
  const auto forwartimer = ogl::begintimer("forward", true);
  setforwardmatrices(fov, aspect, farplane);
  const vec2f subbufferdim(float(sys::scrw/SPLITNUM), float(sys::scrh/SPLITNUM));
  const vec2f rcpsubbufferdim = rcp(subbufferdim);
  ogl::bindshader(forward::shader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, finaltex, 0);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 1);
  OGL(UniformMatrix4fv, forward::u_mvp, 1, GL_FALSE, &mvp.vx.x);
  OGL(Uniform2fv, forward::u_subbufferdim, 1, &subbufferdim.x);
  OGL(Uniform2fv, forward::u_rcpsubbufferdim, 1, &rcpsubbufferdim.x);
  renderscene(false);
  ogl::endtimer(forwartimer);
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
  const auto gbuffertimer = ogl::begintimer("gbuffer", true);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw("St2p3", 4, ground);
#endif
  if (indexnum != 0) {
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    ogl::bindfixedshader(ogl::FIXED_COLOR);
    renderscene(true);
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  }
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  ogl::endtimer(gbuffertimer);

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

  const auto hudtimer = ogl::begintimer("hud", true);
  // drawhudgun(fovy, aspect, farplane);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
  ogl::endtimer(hudtimer);
}
} /* namespace rr */
} /* namespace q */

