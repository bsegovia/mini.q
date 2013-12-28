/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso_mc.hpp"

namespace q {
namespace rr {

void drawdf() {
  ogl::pushmatrix();
  ogl::setortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
  const float verts[] = {-1.f,-1.f,0.f, 1.f,-1.f,0.f, -1.f,1.f,0.f, 1.f,1.f,0.f};
  ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::bindshader(ogl::DFRM_SHADER);
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 0, 0, 4, verts);
  ogl::enablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::popmatrix();
}

/*--------------------------------------------------------------------------
 - handle the HUD (console, scores...)
 -------------------------------------------------------------------------*/
static void drawhud(int w, int h, int curfps) {
  auto cmd = con::curcmd();
  ogl::pushmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::pushmode(ogl::PROJECTION);
  ogl::setortho(0.f, float(VIRTW), float(VIRTH), 0.f, -1.f, 1.f);
  ogl::enablev(GL_BLEND);
  OGL(DepthMask, GL_FALSE);
  if (cmd) text::drawf("> %s_", 20, 1570, cmd);
  ogl::popmode(ogl::PROJECTION);
  ogl::popmode(ogl::MODELVIEW);

  OGL(DepthMask, GL_TRUE);
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

  ogl::enablev(GL_CULL_FACE);
#if 0
  const int rtime = game::reloadtime(game::player.gunselect);
  if (game::player.lastaction &&
      game::player.lastattackgun==game::player.gunselect &&
      game::lastmillis()-game::player.lastaction<rtime)
    drawhudmodel(7, 18, rtime/18.0f, game::player.lastaction);
  else
#endif
  drawhudmodel(6, 1, 100.f, 0);
  ogl::disablev(GL_CULL_FACE);
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

static const vec3f cubefverts[8] = {
  vec3f(0.f,0.f,0.f)/*0*/, vec3f(0.f,0.f,1.f)/*1*/, vec3f(0.f,1.f,1.f)/*2*/, vec3f(0.f,1.f,0.f)/*3*/,
  vec3f(1.f,0.f,0.f)/*4*/, vec3f(1.f,0.f,1.f)/*5*/, vec3f(1.f,1.f,1.f)/*6*/, vec3f(1.f,1.f,0.f)/*7*/
};
const vec3i icubev[8] = {
  vec3i(0,0,0)/*0*/, vec3i(0,0,1)/*1*/, vec3i(0,1,1)/*2*/, vec3i(0,1,0)/*3*/,
  vec3i(1,0,0)/*4*/, vec3i(1,0,1)/*5*/, vec3i(1,1,1)/*6*/, vec3i(1,1,0)/*7*/
};

static float signed_sphere(vec3f v, float r) { return length(v) - r; }

INLINE float signed_box(vec3f p, vec3f b) {
  const vec3f d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0f) + length(max(d,vec3f(zero)));
}

float map(const vec3f &pos) {
  const auto t = pos-vec3f(1.f,2.f,1.f);
  const auto d0 = signed_sphere(t, 1.f);
  const auto d1 = signed_box(t, vec3f(1.f));
  return max(d1, -d0);
}

struct vertex { vec3f pos, nor; };
static u32 vertnum = 0u, indexnum = 0u;
static vertex *vertices = NULL;
static u32 *indices = NULL;
static bool initialized_m = false;

static void makescene() {
  if (initialized_m) return;
  const iso::grid g(vec3f(0.10f), vec3f(zero), vec3f(32));
  auto m = iso::mc(g, map);
  vertnum = m.m_vertnum;
  indexnum = m.m_indexnum;
  indices = m.m_index;
  m.m_index = NULL;
  vertices = (vertex*) malloc(sizeof(vertex) * vertnum);
  loopi(vertnum) {
    vertices[i].pos = m.m_pos[i];
    vertices[i].nor = m.m_nor[i];
  }
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

void frame(int w, int h, int curfps) {
  const float farplane = 100.f;
  const float fovy = fov * float(sys::scrh) / float(sys::scrw);
  const float aspect = float(sys::scrw) / float(sys::scrh);
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  ogl::matrixmode(ogl::MODELVIEW);
  transplayer();

  const float verts[] = {
    -100.f, -100.f, -100.f, 0.f, -100.f,
    +100.f, -100.f, +100.f, 0.f, -100.f,
    -100.f, +100.f, -100.f, 0.f, +100.f,
    +100.f, +100.f, +100.f, 0.f, +100.f
  };
  ogl::bindfixedshader(ogl::DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 2, 0, 4, verts);

  makescene();
  if (vertnum != 0) {
    ogl::bindfixedshader(ogl::COLOR);
    ogl::immdrawelememts("Tip3c3", indexnum, indices, &vertices[0].pos[0]);
  }

  drawhud(w,h,0);
  drawhudgun(fovy, aspect, farplane);
}
} /* namespace rr */
} /* namespace q */

