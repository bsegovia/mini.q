/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.cpp -> stub to run the game
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

#if 0
#include "ui.hpp"
#include <GL/gl.h>
#endif

namespace q {
IVARF(grabmouse, 0, 0, 1, SDL_WM_GrabInput(grabmouse ? SDL_GRAB_ON : SDL_GRAB_OFF););

void start(int argc, const char *argv[]) {
  con::out("init: memory debugger");
  sys::memstart();
  con::out("init: tasking system");
  const u32 threadnum = sys::threadnumber() - 1;
  con::out("init: tasking system: %d threads created", threadnum);
  task::start(&threadnum, 1);
  int fs = 0;
  rangei(1, argc) if (argv[i][0]=='-') switch (argv[i][1]) {
    case 't': fs = 0; break;
    case 'w': sys::scrw  = atoi(&argv[i][2]); break;
    case 'h': sys::scrh  = atoi(&argv[i][2]); break;
    default: con::out("unknown commandline option");
  } else con::out("unknown commandline argument");
  if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0) sys::fatal("SDL failed");

  rr::VIRTH = rr::VIRTW * float(sys::scrh) / float(sys::scrw);
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

  con::out("init: md2 models");
  md2::start();
  con::out("init: renderer");
  rr::start();
  con::out("init: isosurface module");
  iso::start();
  script::execfile("data/keymap.q");
  script::execfile("data/autoexec.q");
}

#if 0
static u32 leftbutton = 0, middlebutton = 0, rightbutton = 0;

static void gui() {
  // ui:: states
  bool checked1 = false;
  bool checked2 = false;
  bool checked3 = true;
  bool checked4 = false;
  float value1 = 50.f;
  float value2 = 30.f;
  int scrollarea1 = 0;
  int scrollarea2 = 0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Mouse states
  unsigned char mousebutton = 0;
  int mousex; int mousey;
  SDL_GetMouseState(&mousex, &mousey);
  mousey = sys::scrh - mousey;
  int toggle = 0;
  if( leftbutton) mousebutton |= ui::MBUT_LEFT;

  // Draw UI
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  float projection[16] = {
    2.f/sys::scrw, 0.f, 0.f,  0.f,
    0.f, 2.f/sys::scrh,  0.f,  0.f,
    0.f,  0.f, -2.f, 0.f,
    -1.f, -1.f,  -1.f,  1.f
  };
  glLoadMatrixf(projection);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  ui::beginframe(mousex, mousey, mousebutton, mscroll);

  ui::beginscrollarea("scroll area", 10, 10, sys::scrw / 5, sys::scrh - 20, &scrollarea1);
  ui::separatorline();
  ui::separator();

  ui::button("button");
  ui::button("disabled button", false);
  ui::item("item");
  ui::item("disabled item", false);
  toggle = ui::check("checkbox", checked1);
  if (toggle)
    checked1 = !checked1;
  toggle = ui::check("disabled checkbox", checked2, false);
  if (toggle)
    checked2 = !checked2;
  toggle = ui::collapse("collapse", "subtext", checked3);
  if (checked3)
  {
    ui::indent();
    ui::label("collapsible element");
    ui::unindent();
  }
  if (toggle)
    checked3 = !checked3;
  toggle = ui::collapse("disabled collapse", "subtext", checked4, false);
  if (toggle)
    checked4 = !checked4;
  ui::label("label");
  ui::value("value");
  ui::slider("slider", &value1, 0.f, 100.f, 1.f);
  ui::slider("disabled slider", &value2, 0.f, 100.f, 1.f, false);
  ui::indent();
  ui::label("indented");
  ui::unindent();
  ui::label("unindented");

  ui::endscrollarea();

  ui::beginscrollarea("scroll area", 20 + sys::scrw / 5, 500, sys::scrw / 5, sys::scrh - 510, &scrollarea2);
  ui::separatorline();
  ui::separator();
  for (int i = 0; i < 100; ++i) ui::label("a wall of text");

  ui::endscrollarea();
  ui::endframe();

  ui::drawtext(30 + sys::scrw / 5 * 2, sys::scrh - 20, ui::ALIGN_LEFT, "free text",  ui::rgba(32,192, 32,192));
  ui::drawtext(30 + sys::scrw / 5 * 2 + 100, sys::scrh - 40, ui::ALIGN_RIGHT, "free text",  ui::rgba(32, 32, 192, 192));
  ui::drawtext(30 + sys::scrw / 5 * 2 + 50, sys::scrh - 60, ui::ALIGN_CENTER, "free text",  ui::rgba(192, 32, 32,192));

  ui::drawline(30 + sys::scrw / 5 * 2, sys::scrh - 80, 30 + sys::scrw / 5 * 2 + 100, sys::scrh - 60, 1.f, ui::rgba(32,192, 32,192));
  ui::drawline(30 + sys::scrw / 5 * 2, sys::scrh - 100, 30 + sys::scrw / 5 * 2 + 100, sys::scrh - 80, 2.f, ui::rgba(32, 32, 192, 192));
  ui::drawline(30 + sys::scrw / 5 * 2, sys::scrh - 120, 30 + sys::scrw / 5 * 2 + 100, sys::scrh - 100, 3.f, ui::rgba(192, 32, 32,192));

  ui::drawroundedrect(30 + sys::scrw / 5 * 2, sys::scrh - 240, 100, 100, 5.f, ui::rgba(32,192, 32,192));
  ui::drawroundedrect(30 + sys::scrw / 5 * 2, sys::scrh - 350, 100, 100, 10.f, ui::rgba(32, 32, 192, 192));
  ui::drawroundedrect(30 + sys::scrw / 5 * 2, sys::scrh - 470, 100, 100, 20.f, ui::rgba(192, 32, 32,192));

  ui::drawrect(30 + sys::scrw / 5 * 2, sys::scrh - 590, 100, 100, ui::rgba(32, 192, 32, 192));
  ui::drawrect(30 + sys::scrw / 5 * 2, sys::scrh - 710, 100, 100, ui::rgba(32, 32, 192, 192));
  ui::drawrect(30 + sys::scrw / 5 * 2, sys::scrh - 830, 100, 100, ui::rgba(192, 32, 32,192));

  ui::rendergldraw(sys::scrw, sys::scrh);
}
#endif

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
#if 0
      {
        if (e.button.button == SDL_BUTTON_LEFT)
          leftbutton = 0;
        else if (e.button.button == SDL_BUTTON_RIGHT)
          rightbutton = 0;
        else if (e.button.button == SDL_BUTTON_MIDDLE)
          middlebutton = 0;
      } break;
#endif
      case SDL_MOUSEBUTTONUP:
#if 0
      if (e.button.button == SDL_BUTTON_LEFT)
          leftbutton = 1;
        else if (e.button.button == SDL_BUTTON_RIGHT)
          rightbutton = 1;
        else if (e.button.button == SDL_BUTTON_MIDDLE)
          middlebutton = 1;
#endif
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

