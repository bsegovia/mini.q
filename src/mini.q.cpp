/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.cpp -> stub to run the game
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "enet/enet.h"
#include <time.h>
#include <SDL2/SDL_image.h>

#include "base/sse.hpp"
#include "base/avx.hpp"
#define TEST_UI 0

#if TEST_UI
#include "ui.hpp"
#include <GL/gl.h>
#endif
namespace q {
namespace sys {
CMD(quit, ARG_1STR);
} /* namespace sys */
} /* namespace q */

namespace q {

VARF(gamespeed, 10, 100, 1000, if (client::multiplayer()) gamespeed = 100);
VARP(minmillis, 0, 5, 1000);
VARP(fullscreen, 0, 0, 1);

static const int SCR_DEFAULTW = 1024;
static const int SCR_DEFAULTH = 768;
static const int SCR_MINW = 320;
static const int SCR_MINH = 200;
static const int SCR_MAXW = 8192;
static const int SCR_MAXH = 8192;
static SDL_Window *screen = NULL;
static SDL_GLContext glcontext = NULL;
static int desktopw = 0, desktoph = 0;
static int renderw = 0, renderh = 0;
static int screenw = 0, screenh = 0;
static bool initwindowpos = false;

VARP(relativemouse, 0, 1, 1);
static bool canrelativemouse = true, isrelativemouse = false;
static void inputgrab(bool on) {
  if (on) {
    SDL_ShowCursor(SDL_FALSE);
    if (canrelativemouse && relativemouse) {
      if (SDL_SetRelativeMouseMode(SDL_TRUE) >= 0) {
        SDL_SetWindowGrab(screen, SDL_TRUE);
        isrelativemouse = true;
      } else {
        SDL_SetWindowGrab(screen, SDL_FALSE);
        canrelativemouse = false;
        isrelativemouse = false;
      }
    }
  } else {
    SDL_ShowCursor(SDL_TRUE);
    if (isrelativemouse) {
      SDL_SetWindowGrab(screen, SDL_FALSE);
      SDL_SetRelativeMouseMode(SDL_FALSE);
      isrelativemouse = false;
    }
  }
}

VARF(grabmouse, 0, 0, 1, inputgrab(grabmouse!=0));

static void setupscreen() {
  if (glcontext) {
    SDL_GL_DeleteContext(glcontext);
    glcontext = NULL;
  }
  if (screen) {
    SDL_DestroyWindow(screen);
    screen = NULL;
  }

  SDL_DisplayMode desktop;
  if (SDL_GetDesktopDisplayMode(0, &desktop) < 0)
    sys::fatal("failed querying desktop display mode: %s", SDL_GetError());
  desktopw = desktop.w;
  desktoph = desktop.h;

  if (sys::scrh < 0) sys::scrh = SCR_DEFAULTH;
  if (sys::scrw < 0) sys::scrw = (sys::scrh*desktopw)/desktoph;
  sys::scrw = min(sys::scrw, desktopw);
  sys::scrh = min(sys::scrh, desktoph);

  int winx = SDL_WINDOWPOS_UNDEFINED;
  int winy = SDL_WINDOWPOS_UNDEFINED;
  int winw = sys::scrw;
  int winh = sys::scrh;
  int flags = 0;
  if (fullscreen) {
    winw = desktopw;
    winh = desktoph;
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    initwindowpos = true;
  }

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  const int winflags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
                       SDL_WINDOW_INPUT_FOCUS |
                       SDL_WINDOW_MOUSE_FOCUS | flags;
  screen = SDL_CreateWindow("mini.q", winx, winy, winw, winh, winflags);
  if (!screen)
    sys::fatal("failed to create OpenGL window: %s", SDL_GetError());

  SDL_SetWindowMinimumSize(screen, SCR_MINW, SCR_MINH);
  SDL_SetWindowMaximumSize(screen, SCR_MAXW, SCR_MAXH);

  static const struct {int major, minor;} coreversions[] = {
    {4,0}, {3,3}, {3,2}, {3,1}, {3,0}
  };
  loopi(sizeof(coreversions)/sizeof(coreversions[0])) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, coreversions[i].major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, coreversions[i].minor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    glcontext = SDL_GL_CreateContext(screen);
    if (glcontext) break;
  }
  if (!glcontext) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
    glcontext = SDL_GL_CreateContext(screen);
    if (!glcontext)
      sys::fatal("failed to create OpenGL context: %s", SDL_GetError());
  }

  SDL_GetWindowSize(screen, &screenw, &screenh);
  renderw = min(sys::scrw, screenw);
  renderh = min(sys::scrh, screenh);
}

// static const float CELLSIZE = 0.2f;
void start(int argc, const char *argv[]) {
  bool dedicated = false;
  int uprate = 0, maxcl = 4;
  const char *master = NULL;
  const char *sdesc = "", *ip = "", *passwd = "";
  rangei(1, argc) {
    const char *a = &argv[i][2];
    if (argv[i][0]=='-') switch (argv[i][1]) {
      case 'd': dedicated = true; break;
      case 't': fullscreen = 0; break;
      case 'w': sys::scrw  = atoi(a); break;
      case 'h': sys::scrh  = atoi(a); break;
      case 'u': uprate = atoi(a); break;
      case 'n': sdesc  = a; break;
      case 'i': ip     = a; break;
      case 'm': master = a; break;
      case 'p': passwd = a; break;
      case 'c': maxcl  = atoi(a); break;
      default:  con::out("unknown commandline option");
    } else
      con::out("unknown commandline argument");
  }
#if !defined(NDEBUG)
  const int par = SDL_INIT_NOPARACHUTE;
#else
  const int par = 0;
#endif

  if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_AUDIO|par)<0)
    sys::fatal("init: failed to initialize SDL");

 SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "0");
 #if !defined(WIN32) && !defined(__APPLE__)
 SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
 #endif

  // load support for the JPG and PNG image formats
  con::out("init: sdl image");
  const auto flags = IMG_INIT_JPG | IMG_INIT_PNG;
  const auto initted = IMG_Init(flags);
  if ((initted&flags) != flags) {
    con::out("IMG_Init: Failed to init required jpg and png support!\n");
      con::out("IMG_Init: %s\n", IMG_GetError());
      sys::fatal("IMG_Init failed");
  }

  con::out("init: memory debugger");
  sys::memstart();

  con::out("init: tasking system");
#if defined(__X86__)
  // flush to zero and no denormals
  _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));
#endif
  const u32 threadnum = sys::threadnumber() - 1;
  con::out("init: tasking system: %d threads created", threadnum);
  task::start(&threadnum, 1);

  con::out("init: video: sdl");
  rr::VIRTH = rr::VIRTW * float(sys::scrh) / float(sys::scrw);
  setupscreen();
  SDL_ShowCursor(SDL_FALSE);
  SDL_StopTextInput();

  con::out("init: video: mode");
  sys::initendiancheck();

  con::out("net: enet");
  if (enet_initialize()<0)
    sys::fatal("enet: unable to initialise network module");
  con::out("net: client");
  game::initclient();
  con::out("net: server");
  server::init(dedicated, uprate, sdesc, ip, master, passwd, maxcl);  // never returns if dedicated

  con::out("init: video: ogl");
  ogl::start(sys::scrw, sys::scrh);
  con::out("init: video: shader system");
  shaders::start();

  con::out("init: video: misc");
  sys::keyrepeat(true);

  con::out("init: sound");
  sound::start();
  con::out("init: md2 models");
  md2::start();
  con::out("init: renderer");
  rr::start();
  con::out("init: isosurface module");
  iso::start();
  inputgrab(false);

  con::out("script");
  menu::newm("frags\tpj\tping\tteam\tname");
  menu::newm("ping\tplr\tserver");
  script::execscript("data/keymap.lua");
  script::execscript("data/menus.lua");
  script::execscript("data/sounds.lua");
  script::execscript("data/autoexec.lua");

  con::out("localconnect");
  server::localconnect();
  client::changemap("metl3");
}

static void playerpos(int x, int y, int z) {game::player1->o = vec3f(vec3i(x,y,z));}
static void playerypr(int x, int y, int z) {game::player1->ypr = vec3f(vec3i(x,y,z));}
CMD(playerpos, ARG_3INT);
CMD(playerypr, ARG_3INT);
VAR(savepos, 0, 1, 1);

static void debug() {script::execfile("debug.q");}
CMD(debug, ARG_NONE);

void finish() {
  if (savepos) {
    auto f = fopen("pos.q", "wb");
    const auto pos = vec3i(game::player1->o);
    const auto ypr = vec3i(game::player1->ypr);
    fprintf(f, "playerpos %d %d %d\n", pos.x, pos.y, pos.z);
    fprintf(f, "playerypr %d %d %d\n", ypr.x, ypr.y, ypr.z);
    fclose(f);
  }
#if !defined(RELEASE)
  game::zapdynent(game::player1);
  game::cleanmonsters();
  iso::finish();
  rr::finish();
  md2::finish();
  shaders::finish();
  text::finish();
  sound::finish();
  menu::finish();
  ogl::finish();
  task::finish();
  con::finish();
  script::finish();
#endif
}

#if TEST_UI
static u32 leftbutton = 0, middlebutton = 0, rightbutton = 0;
static bool checked1 = false;
static bool checked2 = false;
static bool checked3 = true;
static bool checked4 = false;
static float value1 = 50.f;
static float value2 = 30.f;
static int scrollarea1 = 0;
static int scrollarea2 = 0;

static void gui() {
  // ui:: states

  OGL(ClearColor, 0.8f, 0.8f, 0.8f, 1.f);
  OGL(Enable, GL_BLEND);
  OGL(BlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  OGL(Disable, GL_DEPTH_TEST);
  OGL(Disable, GL_CULL_FACE);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Mouse states
  unsigned char mousebutton = 0;
  int mousex; int mousey;
  SDL_GetMouseState(&mousex, &mousey);
  mousey = sys::scrh - mousey;
  int toggle = 0;
  if (leftbutton) mousebutton |= ui::MBUT_LEFT;

  // Draw UI
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setortho(0.f, sys::scrw, 0.f, sys::scrh, -1.f, 1.f);
  ogl::matrixmode(ogl::MODELVIEW);
  ogl::identity();

  ui::beginframe(mousex, mousey, mousebutton, 0);
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
  ui::draw(sys::scrw, sys::scrh);
}

INLINE void mainloop() {
  static int frame = 0;
  const auto millis = sys::millis()*double(gamespeed)/100.0;
  static float fps = 30.0f;
  fps = (1000.0f/float(game::curtime())+fps*50.f)/51.f;
  SDL_GL_SwapWindow(screen);
  ogl::beginframe();
  gui();
  SDL_Event e;
  int lasttype = 0, lastbut = 0;
  while (SDL_PollEvent(&e)) {
    if (frame == 1) continue;
    switch (e.type) {
      case SDL_QUIT: sys::quit(); break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        con::keypress(e.key.keysym.sym, e.key.state==SDL_PRESSED?1:0, e.key.keysym.unicode);
      break;
      case SDL_MOUSEMOTION: game::mousemove(e.motion.xrel, e.motion.yrel); break;
      case SDL_MOUSEBUTTONDOWN: {
        if (e.button.button == SDL_BUTTON_LEFT)
          leftbutton = 1;
        else if (e.button.button == SDL_BUTTON_RIGHT)
          rightbutton = 1;
        else if (e.button.button == SDL_BUTTON_MIDDLE)
          middlebutton = 1;
      } break;
      case SDL_MOUSEBUTTONUP:
      if (e.button.button == SDL_BUTTON_LEFT)
          leftbutton = 0;
        else if (e.button.button == SDL_BUTTON_RIGHT)
          rightbutton = 0;
        else if (e.button.button == SDL_BUTTON_MIDDLE)
          middlebutton = 0;
        if (lasttype==e.type && lastbut==e.button.button) break;
        con::keypress(-e.button.button, e.button.state, 0);
        lasttype = e.type;
        lastbut = e.button.button;
      break;
    }
  }
  ogl::endframe();
}
#endif
static int ignore = 5;

VARP(fov, 30, 90, 160);
VARP(farplane, 50, 100, 1000);

static void computetarget() {
  const float w = float(sys::scrw), h = float(sys::scrh);
  const vec4f nc(w/2,h/2,1.f,1.f);
  const vec4f world = game::invmvpmat * nc;
  game::setworldpos(world.xyz()/world.w);
}

INLINE void mainloop() {
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
  SDL_GL_SwapWindow(screen);
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
}
extern "C" {
int main(int argc, const char **argv) {
  q::start(argc, argv);
  return q::run();
}
}
