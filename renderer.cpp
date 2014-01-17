/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso.hpp"
#include "csg.hpp"

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
  ogl::setortho(0.f, scr.x, scr.y, 0.f, -1.f, 1.f);
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
  setphysicalhud();
  text::displaywidth(text::fontdim().x);
  if (cmd) text::drawf("> %s_", vec2f(8.f, scr.y-50.f), cmd);
  con::render();
  if (showstats) {
    vec2f textpos(scr.x-400.f, scr.y-50.f);
    const vec3f player = game::player.o;
    text::drawf("x: %f y: %f z: %f", textpos, player.x, player.y, player.z);
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
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

static csg::node *n = NULL;
static float map(const vec3f &pos) {
  if (n == NULL) n = csg::makescene();
  return csg::dist(pos, n);
}
typedef pair<vec3f,vec3f> vertex;
static u32 vertnum = 0u, indexnum = 0u;
static vector<vertex> vertices;
static vector<u32> indices;
static bool initialized_m = false;

void start() {}
void finish() {
  destroyscene(n);
  vector<vertex>().moveto(vertices);
  vector<u32>().moveto(indices);
}
static const float CELLSIZE = 0.4f, GRIDJITTER = 0.01f;
static void makescene() {
  if (initialized_m) return;
  const float start = sys::millis();
  auto m = iso::dc_mesh(vec3f(GRIDJITTER), 256, CELLSIZE, map);
  loopi(m.m_vertnum) vertices.add(makepair(m.m_pos[i], m.m_nor[i]));
  loopi(m.m_indexnum) indices.add(m.m_index[i]);
  vertnum = m.m_vertnum;
  indexnum = m.m_indexnum;
  const auto duration = sys::millis() - start;
  assert(indexnum%3 == 0);
  con::out("elapsed %f ms ", duration);
  con::out("tris %i verts %i", indexnum/3, vertnum);
  initialized_m = true;
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

void frame(int w, int h, int curfps) {
  const float farplane = 100.f;
  const float aspect = float(sys::scrw) / float(sys::scrh);
  const float fovy = fov / aspect;
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  ogl::matrixmode(ogl::MODELVIEW);
  transplayer();
  const float verts[] = {
    -100.f, -100.f, -100.f, 0.f, -100.f,
    -100.f, +100.f, -100.f, 0.f, +100.f,
    +100.f, -100.f, +100.f, 0.f, -100.f,
    +100.f, +100.f, +100.f, 0.f, +100.f
  };
  ogl::bindfixedshader(ogl::DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 2, 0, 4, verts);

  makescene();
  if (vertnum != 0) {
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
    ogl::bindfixedshader(ogl::COLOR);
    ogl::immdrawelememts("Tip3c3", indexnum, &indices[0], &vertices[0].first[0]);
    if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
  }

  drawhudgun(fovy, aspect, farplane);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
}
} /* namespace rr */
} /* namespace q */

