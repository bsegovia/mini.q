/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.cpp -> stub to run the game
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "rt.hpp"
#include "enet/enet.h"
#include <time.h>
#include <SDL2/SDL_image.h>

namespace q {
namespace sys {
CMD(quit);
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

static void outputcpufeatures() {
  using namespace sys;
  fixedstring features("cpu: ");
  loopi(CPU_FEATURE_NUM) {
    const auto name = featurename(cpufeature(i));
    const auto hasit = hasfeature(cpufeature(i));
    strcat_s(features, hasit ? name : "");
    if (hasit && i != int(CPU_FEATURE_NUM)-1) strcat_s(features, " ");
  }
  con::out(features.c_str());
}

void swap() {
  SDL_GL_SwapWindow(screen);
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

 outputcpufeatures();

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

  con::out("init: csg module");
  csg::start();
  script::execscript("data/csg.lua");
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
CMD(playerpos);
CMD(playerypr);
VAR(savepos, 0, 1, 1);

static void debug() {script::execscript("debug.lua");}
CMD(debug);

void finish() {
  if (savepos) {
    auto f = fopen("pos.lua", "wb");
    const auto pos = vec3i(game::player1->o);
    const auto ypr = vec3i(game::player1->ypr);
    fprintf(f, "playerpos(%d, %d, %d)\n", pos.x, pos.y, pos.z);
    fprintf(f, "playerypr(%d, %d, %d)\n", ypr.x, ypr.y, ypr.z);
    fclose(f);
  }
#if !defined(RELEASE)
  game::zapdynent(game::player1);
  game::cleanmonsters();
  rt::finish();
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

VARP(fov, 30, 90, 160);
VARP(farplane, 50, 100, 1000);
} /* namespace q */

