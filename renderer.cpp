/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "ogl.hpp"
#include "text.hpp"
#include "renderer.hpp"

namespace q {
namespace rr {

void drawhud(int w, int h, int curfps) {
  auto cmd = con::curcmd();
  ogl::pushmatrix();
  ogl::ortho(0.f, float(VIRTW), float(VIRTH), 0.f, -1.f, 1.f);
  ogl::enablev(GL_BLEND);
  OGL(DepthMask, GL_FALSE);
  if (cmd) text::drawf("> %s_", 20, 1570, cmd);
  ogl::popmatrix();

  OGL(DepthMask, GL_TRUE);
  ogl::disablev(GL_BLEND);
  ogl::enablev(GL_DEPTH_TEST);
}

void drawframe(int w, int h, int curfps) {
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawhud(w,h,0);
}

} /* namespace rr */
} /* namespace q */

