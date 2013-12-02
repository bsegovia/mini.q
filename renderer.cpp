/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "math.hpp"
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

void drawframe(int w, int h, int curfps) {
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawdf();
  drawhud(w,h,0);
}

} /* namespace rr */
} /* namespace q */

