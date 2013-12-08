/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace rr {

void hud(int w, int h, int curfps) {
  auto cmd = con::curcmd();
  ogl::pushmatrix();
  ogl::ortho(0.f, float(VIRTW), float(VIRTH), 0.f, -1.f, 1.f);
  ogl::enablev(GL_BLEND);
  OGL(DepthMask, GL_FALSE);
  if (cmd) text::drawf("> %s_", 20, 1570, cmd);
  ogl::popmatrix();

  OGL(DepthMask, GL_TRUE);
  ogl::disable(GL_BLEND);
  ogl::enable(GL_DEPTH_TEST);
}

void drawdf() {
  ogl::pushmatrix();
  ogl::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
  const array<float,3> verts[] = {
    array<float,3>(-1.f, -1.f, 0.f),
    array<float,3>(1.f, -1.f, 0.f),
    array<float,3>(-1.f, 1.f, 0.f),
    array<float,3>(1.f, 1.f, 0.f),
  };
  ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::bindshader(ogl::DFRM_SHADER);
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 0, 0, 4, &verts[0][0]);
  ogl::enablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::popmatrix();
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

static void transplayer(void) {
  using namespace game;
  ogl::identity();
#if 1
  printf("\r%f %f %f %f %f %f",
      player.o.x, player.o.y, player.o.z,
      player.roll, player.pitch, player.yaw);
#endif
  ogl::rotate(player.roll, vec3f(0.f,0.f,1.f));
  ogl::rotate(player.pitch, vec3f(1.f,0.f,0.f));
  ogl::rotate(player.yaw, vec3f(0.f,1.f,0.f));
  ogl::translate(-player.o);
}

void frame(int w, int h, int curfps) {
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//  drawdf();

  const float farplane = 100.f;
  const float fovy = fov * float(sys::scrh) / float(sys::scrw);
  const float aspect = float(sys::scrw) / float(sys::scrh);

  ogl::pushmode(ogl::PROJECTION);
  ogl::identity();
  ogl::perspective(fovy, aspect, 0.15f, farplane);
  ogl::pushmode(ogl::MODELVIEW);
  transplayer();

  const array<float,3> verts[] = {
    array<float,3>(-100.f, -10.f, -100.f),
    array<float,3>( 100.f, -10.f, -100.f ),
    array<float,3>( -100.f, -10.f, 100.f ),
    array<float,3>( 100.f, -10.f, 100.f  ),
  };
  ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::bindfixedshader(0);
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 0, 0, 4, &verts[0][0]);

  ogl::popmode(ogl::MODELVIEW);
  ogl::popmode(ogl::PROJECTION);

  hud(w,h,0);
}

} /* namespace rr */
} /* namespace q */

