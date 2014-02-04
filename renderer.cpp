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
#define DEBUG_UNSPLIT 0
#if DEBUG_UNSPLIT
static u32 unsplittex;
static u32 unsplitbuffer;
#endif
static u32 depthtex, nortex;
static u32 gbuffer;

static struct deferredshadertype : ogl::shadertype {
  u32 u_mvp;
  u32 u_nortex;
  u32 fs_tex;
} deferredshader;

// can be reused by other shaders reusing the same skeleton as deferred shaders
struct deferredshaderbuilder : ogl::shaderbuilder {
  deferredshaderbuilder() :
    shaderbuilder("data/shaders/deferred_vp.glsl",
                  "data/shaders/deferred_fp.glsl",
                  shaders::deferred_vp,
                  shaders::deferred_fp) {}
  virtual void setrules(string &str) {}
  virtual void setuniform(ogl::shadertype &s) {
  auto &shader = static_cast<deferredshadertype&>(s);
    OGLR(shader.u_mvp, GetUniformLocation, shader.program, "u_mvp");
    OGLR(shader.u_nortex, GetUniformLocation, shader.program, "u_nortex");
    OGL(Uniform1i, shader.u_nortex, 0);
  }
  virtual void setvarying(ogl::shadertype &s) {
    auto &shader = static_cast<deferredshadertype&>(s);
    OGL(BindAttribLocation, shader.program, ogl::ATTRIB_POS0, "vs_pos");
#if !defined(__WEBGL__)
    OGL(BindFragDataLocation, shader.program, 0, "rt_col");
#endif // __WEBGL__
  }
};

static void initdeferred() {
  nortex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  depthtex = ogl::maketex("Tf Id Dd Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  OGL(GenFramebuffers, 1, &gbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, nortex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, depthtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init gbuffer framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  if (!NEWE(deferredshaderbuilder)->build(deferredshader, ogl::loadfromfile()))
    ogl::shadererror(true, "deferred shader");

#if DEBUG_UNSPLIT
  unsplittex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  OGL(GenFramebuffers, 1, &unsplitbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, unsplitbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, unsplittex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init debug unsplit framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  // PAS BON
  if (!NEWE(deferredshaderbuilder)->build(unsplitshader, ogl::loadfromfile()))
    ogl::shadererror(true, "deferred shader");
#endif
}

static void cleandeferred() {
  ogl::deletetextures(1, &nortex);
  ogl::deletetextures(1, &depthtex);
  OGL(DeleteFramebuffers, 1, &gbuffer);
  ogl::destroyshader(deferredshader);
#if DEBUG_UNSPLIT
  ogl::deletetextures(1, &unsplittex);
  OGL(DeleteFramebuffers, 1, &unsplitbuffer);
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
  static INLINE screenquad get2D() {
    const screenquad qs = {
      1.f, 1.f, float(sys::scrw), float(sys::scrh),
      1.f, 0.f, float(sys::scrw),              0.f,
      0.f, 1.f,              0.f, float(sys::scrh),
      0.f, 0.f,              0.f,              0.f
    };
    return qs;
  };
  static INLINE screenquad getRect() {
    const float w = float(sys::scrw), h = float(sys::scrh);
    const screenquad qs = {w,h,w,0.f,0.f,h,0.f,0.f};
    return qs;
  };
  float v[16];
};

static void deferred() {
  ogl::bindshader(deferredshader);
  ogl::bindtexture(GL_TEXTURE_RECTANGLE, nortex);
  const auto &mv = ogl::matrix(ogl::MODELVIEW);
  const auto &p  = ogl::matrix(ogl::PROJECTION);
  const auto mvp = p*mv;
  OGL(UniformMatrix4fv, deferredshader.u_mvp, 1, GL_FALSE, &mvp.vx.x);
  ogl::disable(GL_CULL_FACE);
  ogl::immenableflush(false);
  ogl::immdraw("Sp2", 4, screenquad::getRect().v);
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

IVAR(linemode, 0, 0, 1);
IVAR(useframebuffer, 0, 0, 1);

void frame(int w, int h, int curfps) {
  const float farplane = 100.f;
  const float aspect = float(sys::scrw) / float(sys::scrh);
  const float fovy = fov / aspect;
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  ogl::matrixmode(ogl::MODELVIEW);
  transplayer();
  const float ground[] = {
    -100.f, -100.f, -100.f, 0.f, -100.f,
    -100.f, +100.f, -100.f, 0.f, +100.f,
    +100.f, -100.f, +100.f, 0.f, -100.f,
    +100.f, +100.f, +100.f, 0.f, +100.f
  };

  makescene();

  // render the scene into the frame buffer
  if (useframebuffer)
    OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw("St2p3", 4, ground);
  if (vertnum != 0) {
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    ogl::bindfixedshader(ogl::FIXED_COLOR);
    ogl::immdrawelememts("Tip3c3", indexnum, &indices[0], &vertices[0].first[0]);
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  }
  if (useframebuffer)
    OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);

  // blit the normal buffer into screen
  if (useframebuffer) {
    setscreentransform();
#if 0
    ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
    ogl::bindtexture(GL_TEXTURE_2D, nortex);
    ogl::disable(GL_CULL_FACE);
    ogl::immdraw("St2p2", 4, screenquad::get2D().v);
    ogl::enable(GL_CULL_FACE);
#else
    deferred();
#endif
    popscreentransform();
  } else
    drawhudgun(fovy, aspect, farplane);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
}
} /* namespace rr */
} /* namespace q */

