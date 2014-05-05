/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - main.cpp -> main entry point for the client
 -------------------------------------------------------------------------*/
#include "base/sys.hpp"
#include "base/math.hpp"
#include "game.hpp"
#include "mini.q.hpp"
#include <time.h>

namespace q {

static void computetarget() {
  const float w = float(sys::scrw), h = float(sys::scrh);
  const vec4f nc(w/2,h/2,1.f,1.f);
  const vec4f world = game::invmvpmat * nc;
  game::setworldpos(world.xyz()/world.w);
}

INLINE void mainloop() {
  static int ignore = 5;
  auto millis = sys::millis()*double(gamespeed)/100.0;
  if (millis-game::lastmillis()>200.0) game::setlastmillis(millis-200.0);
  else if (millis-game::lastmillis()<1) game::setlastmillis(millis-1.0);
#if !defined(__JAVASCRIPT__)
  if (millis-game::lastmillis()<double(minmillis))
    SDL_Delay(minmillis-(int(millis)-int(game::lastmillis())));
#endif // __JAVASCRIPT__
  const auto fovy = float(fov), ffar = float(farplane);
  game::setmatrices(fovy, ffar, float(sys::scrw), float(sys::scrh));
  computetarget();
  game::updateworld(int(millis));
  if (!demo::playing())
    server::slice(int(time(NULL)), 0);
  static double fps = 30.0;
  fps = (1000.0/game::curtime()+fps*50.0)/51.0;
  swap();
  ogl::beginframe();
  sound::updatevol();
  rr::frame(sys::scrw, sys::scrh, int(fps));
  ogl::endframe();
  SDL_Event event;
  int lasttype = 0, lastbut = 0;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT: sys::quit(); break;
      case SDL_TEXTINPUT:
        con::processtextinput(event.text.text);
      break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        con::keypress(event.key.keysym.sym, event.key.state==SDL_PRESSED);
      break;
      case SDL_MOUSEMOTION:
        if (ignore) {
          ignore--;
          break;
        }
        game::mousemove(event.motion.xrel, event.motion.yrel);
      break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        // why?? get event twice without it
        if (lasttype==int(event.type) && lastbut==event.button.button) break;
        con::keypress(-event.button.button, event.button.state!=0);
        lasttype = event.type;
        lastbut = event.button.button;
      break;
    }
  }
}

static int run() {
  game::setlastmillis(sys::millis() * double(gamespeed)/100.0);
  for (;;) q::mainloop();
  return 0;
}
} /* namespace q */
extern "C" {
int main(int argc, const char **argv) {
  q::start(argc, argv);
  return q::run();
}
}

