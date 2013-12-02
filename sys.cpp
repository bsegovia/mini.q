/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sys.cpp -> implements platform specific code, stdlib and main function
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "script.hpp"
#include "renderer.hpp"
#include "game.hpp"
#include "ogl.hpp"
#include "sys.hpp"

namespace q {
namespace sys {
char *path(char *s) {
  for (char *t = s; (t = strpbrk(t, "/\\")) != 0; *t++ = PATHDIV);
  return s;
}

char *loadfile(const char *fn, int *size) {
  auto f = fopen(fn, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  u32 len = ftell(f);
  fseek(f, 0, SEEK_SET);
  auto buf = (char *) malloc(len+1);
  if (!buf) return NULL;
  buf[len] = 0;
  size_t rlen = fread(buf, 1, len, f);
  fclose(f);
  if (len!=rlen || len<=0) {
    free(buf);
    return NULL;
  }
  if (size!=NULL) *size = len;
  return buf;
}

static void quit(const char *msg = NULL) {
  if (msg && strlen(msg)) {
#if defined(__WIN32__)
    MessageBox(NULL, msg, "cube fatal error", MB_OK|MB_SYSTEMMODAL);
#else
    printf("%s\n", msg);
#endif // __WIN32__
  } else {
    ogl::end();
  }
  SDL_Quit();
  exit(msg && strlen(msg) ? EXIT_FAILURE : EXIT_SUCCESS);
}
CMD(quit, "s");

void fatal(const char *s, const char *o) {
  sprintf_sd(m)("%s%s (%s)\n", s, o, SDL_GetError());
  quit(m);
}

void keyrepeat(bool on) {
  SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0, SDL_DEFAULT_REPEAT_INTERVAL);
}

#if defined(__WIN32__)
float millis() {
  LARGE_INTEGER freq, val;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&val);ss
  static double first = double(val.QuadPart) / double(freq.QuadPart) * 1e3;
  return float(double(val.QuadPart) / double(freq.QuadPart) * 1e3 - first);
}
#else
float millis() {
  struct timeval tp; gettimeofday(&tp,NULL);
  static double first = double(tp.tv_sec)*1e3 + double(tp.tv_usec)*1e-3;
  return float(double(tp.tv_sec)*1e3 + double(tp.tv_usec)*1e-3 - first);
}
#endif

int scrw = 800, scrh = 600, grabmouse = 0;

static INLINE void end(void) { quit(); }

void start(int argc, const char *argv[]) {
  int fs = 0;
  rangei(1, argc) if (argv[i][0]=='-') switch (argv[i][1]) {
    case 't': fs     = 0; break;
    case 'w': scrw  = atoi(&argv[i][2]); break;
    case 'h': scrh  = atoi(&argv[i][2]); break;
    default: con::out("unknown commandline option");
  } else con::out("unknown commandline argument");
  if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0) fatal("SDL failed");

  con::out("init: video: sdl");
  if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0) fatal("SDL video failed");
  SDL_WM_GrabInput(grabmouse ? SDL_GRAB_ON : SDL_GRAB_OFF);

  con::out("init: video: mode");
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (!SDL_SetVideoMode(scrw, scrh, 0, SDL_OPENGL|fs)) fatal("OpenGL failed");

  con::out("init: video: ogl");
  ogl::start(scrw, scrh);

  con::out("init: video: misc");
  SDL_WM_SetCaption("mini.q", NULL);
  sys::keyrepeat(true);
  SDL_ShowCursor(0);
  script::execfile("data/keymap.q");
}

static INLINE void mainloop() {
  const auto millis = sys::millis()*game::speed/100.f;
  if (millis-game::lastmillis > 200.f) game::lastmillis = millis-200.f;
  else if (millis-game::lastmillis < 1.f) game::lastmillis = millis-1.f;
  static float fps = 30.0f;
  fps = (1000.0f/game::curtime+fps*50.f)/51.f;
  SDL_GL_SwapBuffers();
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  rr::drawframe(scrw, scrh, int(fps));
  SDL_Event e;
  int lasttype = 0, lastbut = 0;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: quit(); break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        con::keypress(e.key.keysym.sym, e.key.state==SDL_PRESSED, e.key.keysym.unicode);
      break;
      case SDL_MOUSEMOTION: game::mousemove(e.motion.xrel, e.motion.yrel); break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (lasttype==e.type && lastbut==e.button.button) break;
        // on_keypress(-e.button.button, e.button.state!=0, 0);
        lasttype = e.type;
        lastbut = e.button.button;
      break;
    }
  }
}
int run() {
  game::lastmillis = sys::millis() * game::speed/100.f;
  for (;;) q::sys::mainloop();
  return 0;
}
} /* namespace sys */
} /* namespace q */

