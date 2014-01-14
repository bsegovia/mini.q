/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso.hpp"

namespace q {
namespace rr {

#if 0
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
#endif

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
  if (cmd) {
    text::charwidth(64);
    text::thickness(0.5f);
    text::drawf("> %s_", 32, 400, cmd);
  }
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

INLINE float U(float d0, float d1) { return min(d0, d1); }
INLINE float D(float d0, float d1) { return max(d0,-d1); }
INLINE float sd_sphere(const vec3f &v, float r) { return length(v) - r; }
INLINE float sd_box(const vec3f &p, const vec3f &b) {
  const vec3f d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0f) + length(max(d,vec3f(zero)));
}
INLINE float sd_cyl(const vec3f &p, const vec2f &cxz, float r) {
  return length(p.xz()-cxz) - r;
}
INLINE float sd_plane(const vec3f &p, const vec4f &n) {
  return dot(p,n.xyz())+n.w;
}
INLINE float sd_cyl(const vec3f &p, const vec2f &cxz, const vec3f &ryminymax) {
  const auto cyl = sd_cyl(p, cxz, ryminymax.x);
  const auto plane0 = sd_plane(p, vec4f(0.f,1.f,0.f,-ryminymax.y));
  const auto plane1 = sd_plane(p, vec4f(0.f,-1.f,0.f,ryminymax.z));
  return D(D(cyl, plane0), plane1);
}

static float map(const vec3f &pos) {
  const auto t = pos-vec3f(7.f,5.f,7.f);
  const auto d0 = sd_sphere(t, 4.2f);
  const auto d1 = sd_box(t, vec3f(4.f));
  auto c = D(d1, d0);
  loopi(16) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2*float(i)+2.f);
    c = U(c, sd_cyl(pos, center, ryminymax));
  }
  return D(c, sd_box(pos-vec3f(2.f,5.f,18.f), vec3f(3.5f,4.f,3.5f)));
}

typedef pair<vec3f,vec3f> vertex;
static u32 vertnum = 0u, indexnum = 0u;
static vector<vertex> vertices;
static vector<u32> indices;
static bool initialized_m = false;

void start() {}
void finish() {
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

  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,0);
  ogl::enable(GL_CULL_FACE);
  drawhudgun(fovy, aspect, farplane);
}
} /* namespace rr */
} /* namespace q */

