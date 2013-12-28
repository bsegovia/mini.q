/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.cpp -> stub to run the game
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
IVARF(grabmouse, 0, 0, 1, SDL_WM_GrabInput(grabmouse ? SDL_GRAB_ON : SDL_GRAB_OFF););

void start(int argc, const char *argv[]) {
  sys::meminit();
  int fs = 0;
  rangei(1, argc) if (argv[i][0]=='-') switch (argv[i][1]) {
    case 't': fs     = 0; break;
    case 'w': sys::scrw  = atoi(&argv[i][2]); break;
    case 'h': sys::scrh  = atoi(&argv[i][2]); break;
    default: con::out("unknown commandline option");
  } else con::out("unknown commandline argument");
  if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0) sys::fatal("SDL failed");

  con::out("init: video: sdl");
  if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0) sys::fatal("SDL video failed");
  SDL_WM_GrabInput(grabmouse ? SDL_GRAB_ON : SDL_GRAB_OFF);

  con::out("init: video: mode");
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (!SDL_SetVideoMode(sys::scrw, sys::scrh, 0, SDL_OPENGL|fs)) sys::fatal("OpenGL failed");
  sys::initendiancheck();

  con::out("init: video: ogl");
  ogl::start(sys::scrw, sys::scrh);

  con::out("init: video: misc");
  SDL_WM_SetCaption("mini.q", NULL);
  sys::keyrepeat(true);
  SDL_ShowCursor(0);
  script::execfile("data/keymap.q");
}

INLINE void mainloop() {
  static int frame = 0;
  const auto millis = sys::millis()*game::speed/100.f;
  static float fps = 30.0f;
  fps = (1000.0f/game::curtime+fps*50.f)/51.f;
  SDL_GL_SwapBuffers();
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (frame++) {
    physics::frame();
    game::updateworld(millis);
  }
  rr::frame(sys::scrw, sys::scrh, int(fps));
  SDL_Event e;
  int lasttype = 0, lastbut = 0;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: sys::quit(); break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        con::keypress(e.key.keysym.sym, e.key.state==SDL_PRESSED?1:0, e.key.keysym.unicode);
      break;
      case SDL_MOUSEMOTION: game::mousemove(e.motion.xrel, e.motion.yrel); break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (lasttype==e.type && lastbut==e.button.button) break;
        con::keypress(-e.button.button, e.button.state, 0);
        lasttype = e.type;
        lastbut = e.button.button;
      break;
    }
  }
}
static int run() {
  game::lastmillis = sys::millis() * game::speed/100.f;
  for (;;) q::mainloop();
  return 0;
}
} /* namespace q */

int main(int argc, const char *argv[]) {
  q::start(argc, argv);
  return q::run();
}

