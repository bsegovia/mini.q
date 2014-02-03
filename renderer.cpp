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
static void setphysicalhud() {
  const auto scr = scrdim();
  ogl::pushmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::pushmode(ogl::PROJECTION);
  ogl::setortho(0.f, scr.x, 0.f, scr.y, -1.f, 1.f);
}
static void popmatrices() {
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
  setphysicalhud();
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
  popmatrices();
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
static u32 depthtex, nortex;
static u32 gbuffer;

static struct deferredshadertype : ogl::shadertype {
  u32 u_mvp;
  u32 u_nortex;
  u32 fs_tex;
} deferredshader;

static void initdeferred() {
  nortex = ogl::maketex("TB I3 D3 B2 Wsr Wtr mn Mn", NULL, sys::scrw, sys::scrh);
  depthtex = ogl::maketex("Tf Id Dd B2 Wsr Wtr mn Mn", NULL, sys::scrw, sys::scrh);
  OGL(GenFramebuffers, 1, &gbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nortex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init gbuffer framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
}

static void cleandeferred() {
  OGL(DeleteTextures, 1, &nortex);
  OGL(DeleteTextures, 1, &depthtex);
  OGL(DeleteFramebuffers, 1, &gbuffer);
  ogl::destroyshader(deferredshader);
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

static void transplayer(void) {
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
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 2, 0, 4, ground);
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
    setphysicalhud();
    ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
    ogl::bindtexture(GL_TEXTURE_2D, nortex);
    ogl::disable(GL_CULL_FACE);
    const float quad[] = {
      1.f, 1.f, float(sys::scrw), float(sys::scrh),
      1.f, 0.f, float(sys::scrw),              0.f,
      0.f, 1.f,              0.f, float(sys::scrh),
      0.f, 0.f,              0.f,              0.f
    };
    ogl::immdraw(GL_TRIANGLE_STRIP, 2, 2, 0, 4, quad);
    ogl::enable(GL_CULL_FACE);
    popmatrices();
  } else
    drawhudgun(fovy, aspect, farplane);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
}
} /* namespace rr */
} /* namespace q */

