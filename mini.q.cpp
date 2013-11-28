/*-------------------------------------------------------------------------
 - mini.q
 - a minimalistic multiplayer FPS
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {

/*-------------------------------------------------------------------------
 - game variables
 -------------------------------------------------------------------------*/
namespace game {
static void mousemove(int dx, int dy) {
}
float lastmillis = 0.f;
float curtime = 1.f;
FVARP(speed, 1.f, 100.f, 1000.f);
}

/*-------------------------------------------------------------------------
 - script
 -------------------------------------------------------------------------*/
namespace script {

enum { FVAR, IVAR, FUN };
template <class T> struct cvar { T *storage,min,max; cb fun; bool persist; };
struct fn { const char *proto; cb fun; };
struct identifier {
  int type;
  union {
    cvar<int> ivar;
    cvar<float> fvar;
    fn fun;
  };
};
static hashtable<identifier> idents;
int ivar(const char *n, int m, int cur, int M, int *ptr, cb fun, bool persist) {
  identifier v = {IVAR}; v.ivar = {ptr, m, M, fun, persist};
  idents.access(n, &v);
  return cur;
}
float fvar(const char *n, float m, float cur, float M, float *ptr, cb fun, bool persist) {
  identifier v = {FVAR}; v.fvar = {ptr, m, M, fun, persist};
  idents.access(n, &v);
  return cur;
}
bool cmd(const char *n, const char *proto, cb fun) {
  identifier v = {FUN}; v.fun = {proto, fun};
  idents.access(n, &v);
  return true;
}

typedef linear_allocator<1024> ctx;
static char *parseword(ctx &c, const char *&p) {
  p += strspn(p, " \t\r");
  if (p[0]=='/' && p[1]=='/') p += strcspn(p, "\n\0");
  if (*p=='[') { // nested string
    const auto left = *p++;
    const auto word = p;
    for (int brak = 1; brak; ) {
      const auto c = *p++;
      if (c==left) brak++;
      else if (c==']') brak--;
      else if (!c) { p--; con::out("missing \"]\""); return NULL; }
    }
    return c.newstring(word, p-word-1);
  }
  auto w = p;
  p += strcspn(p, "; \t\r\n\0");
  if (p-w==0) return NULL;
  return c.newstring(w, p-w);
}

#define VAR(T,S) do {\
  const auto min = id->S##var.min, max = id->S##var.max;\
  auto &s = *id->S##var.storage;\
  if (!w[1][0]) con::out("%s = %" #S, c, s);\
  else {\
    if (min>max) con::out("variable is read-only");\
    else {\
      T i1 = ato##S(w[1]);\
      if (i1<min || i1>max) {\
        i1 = i1<min ? min : max;\
        con::out("range for %s is %"#S"..%"#S,c,min,max);\
      }\
      s = i1;\
    }\
    if (id->fun.fun) id->fun.fun();\
  }\
} while (0)

static void execute(ctx &c, const char *pp, int isdown) {
  auto p = pp;
  const int MAXWORDS = 8;
  char *w[MAXWORDS];

  // for each ; separated statement
  for (bool cont = true; cont;) {
    auto numargs = MAXWORDS;
    loopi(MAXWORDS) {
      w[i] = c.newstring("");
      if (i>numargs) continue;
      auto s = parseword(c,p);
      if (!s) numargs = i;
      w[i] = s;
    }
    loopi(MAXWORDS) if (w[i] == NULL) {
      con::out("out-of-memory for script execution");
      return;
    }

    p += strcspn(p, ";\n\0");
    cont = *p++!=0; // more statements if this isn't the end of the string
    auto ch = w[0];
    if (*ch=='/') ch++; // strip irc-style command prefix
    if (!*ch) continue; // empty statement

    auto id = idents.access(ch);
    if (!id) {
      con::out("unknown command: %s", ch);
      return;
    }

    switch (id->type) {
      case FUN: {
        const auto proto = id->fun.proto;
        const auto arity = strlen(proto);
        loopi(min(arity, numargs-1)) {
          if (proto[i]=='d') memcpy(w[i],&isdown,sizeof(isdown));
#define ARG(T,C,S) if(proto[i]==C){T x=ato##S(w[i]);memcpy(w[i],&x,sizeof(T));}
          ARG(int, 'i', i)
          ARG(float, 'f', f)
#undef ARG
        }
        switch (arity) {
#define C char*
#define PROTO(...) ((int (CDECL*)(__VA_ARGS__))id->fun.fun)
          case 0: PROTO()(); break;
          case 1: PROTO(C)(w[1]); break;
          case 2: PROTO(C,C)(w[1],w[2]); break;
          case 3: PROTO(C,C,C)(w[1],w[2],w[3]); break;
          case 4: PROTO(C,C,C,C)(w[1],w[2],w[3],w[4]); break;
#undef C
#undef PROTO
          default: break;
        }
      }
      case IVAR: VAR(int, i); break;
      case FVAR: VAR(float, f); break;
    }
    c.rewind();
  }
}
#undef VAR
void execute(const char *str, int isdown) {ctx c; execute(c,str,isdown);}
} /* script */

/*-------------------------------------------------------------------------
 - console
 -------------------------------------------------------------------------*/
namespace con {
#if 0
static void con::saycommand(const char *init) {
  SDL_EnableUNICODE(saycommandon = (init!=NULL));
  if (!init) init = "";
  strcpy_s(commandbuf, init);
}
#endif
static circular_buffer<pair<string,float>, 128> buffer;

static void line(const char *sf, bool highlight) {
  auto &cl = buffer.prepend();
  cl.second = game::lastmillis;
  if (highlight) {
    cl.first[0] = '\f';
    cl.first[1] = 0;
    strcat_s(cl.first, sf);
  } else
    strcpy_s(cl.first, sf);
  puts(cl.first);
}

void out(const char *s, ...) {
  sprintf_sdv(sf, s);
  s = sf;
  int n = 0;
  while (strlen(s) > WORDWRAP) { // cut strings to fit on screen
    string t;
    strn0cpy(t, s, WORDWRAP+1);
    line(t, n++!=0);
    s += WORDWRAP;
  }
  line(s, n!=0);
}

void keypress(int code, bool isdown, int cooked) {
#if 0
  if (saycommandon) { // keystrokes go to commandline
    if (isdown) {
      switch (code) {
        case SDLK_RETURN: break;
        case SDLK_BACKSPACE:
        case SDLK_LEFT: {
          for (int i = 0; commandbuf[i]; ++i) if (!commandbuf[i+1]) commandbuf[i] = 0;
          cmd::resetcomplete();
          break;
        }
        case SDLK_UP: if (histpos) strcpy_s(commandbuf, vhistory[--histpos]); break;
        case SDLK_DOWN: if (histpos<vhistory.size()) strcpy_s(commandbuf, vhistory[histpos++]); break;
        case SDLK_TAB: cmd::complete(commandbuf); break;
        default:
          cmd::resetcomplete();
          if (cooked) {
            const char add[] = { char(cooked), 0 };
            strcat_s(commandbuf, add);
          }
      }
    } else {
      if (code==SDLK_RETURN) {
        if (commandbuf[0]) {
          if (vhistory.empty() || strcmp(vhistory.back(), commandbuf))
            vhistory.add(NEWSTRING(commandbuf));  // cap this?
          histpos = vhistory.size();
          if (commandbuf[0]=='/')
            cmd::execute(commandbuf, true);
          else
            client::toserver(commandbuf);
        }
        saycommand(NULL);
      }
      else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else if (!menu::key(code, isdown)) { // keystrokes go to menu
    loopi(numkm) if (keyms[i].code==code) { // keystrokes go to game, lookup in keymap and execute
      string temp;
      strcpy_s(temp, keyms[i].action);
      cmd::execute(temp, isdown);
      return;
    }
  }
#endif
}
} /* namespace con */

/*-------------------------------------------------------------------------
 - main entry / exit points
 -------------------------------------------------------------------------*/
namespace sys {
char *path(char *s) {
  for (char *t = s; (t = strpbrk(t, "/\\")) != 0; *t++ = PATHDIV);
  return s;
}
static void quit(const char *msg = NULL) {
  if (msg) {
#if defined(__WIN32__)
    MessageBox(NULL, msg, "cube fatal error", MB_OK|MB_SYSTEMMODAL);
#else
    printf("%s\n", msg);
#endif // __WIN32__
  }
  SDL_Quit();
  exit(msg ? EXIT_FAILURE : EXIT_SUCCESS);
}
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
  QueryPerformanceCounter(&val);
  return double(val.QuadPart) / float(freq.QuadPart) * 1e3f;
}
#else
float millis() {
  struct timeval tp; gettimeofday(&tp,NULL);
  return float(tp.tv_sec) * 1e3f + float(tp.tv_usec)/1e3f;
}
#endif

static int scr_w = 800, scr_h = 600, grabmouse = 1;
static INLINE void end(void) { quit(); }
static INLINE void start(int argc, const char *argv[]) {
  int fs = 0;
  rangei(1, argc) if (argv[i][0]=='-') switch (argv[i][1]) {
    case 't': fs     = 0; break;
    case 'w': scr_w  = atoi(&argv[i][2]); break;
    case 'h': scr_h  = atoi(&argv[i][2]); break;
    default: con::out("unknown commandline option");
  } else con::out("unknown commandline argument");
  if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO) < 0) fatal("SDL failed");

  con::out("init: video: sdl");
  if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0) fatal("SDL video failed");
  SDL_WM_GrabInput(grabmouse ? SDL_GRAB_ON : SDL_GRAB_OFF);

  con::out("init: video: mode");
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  if (!SDL_SetVideoMode(scr_w, scr_h, 0, SDL_OPENGL|fs)) fatal("OpenGL failed");

  con::out("init: video: misc");
  SDL_WM_SetCaption("mini.q", NULL);
  keyrepeat(true);
  SDL_ShowCursor(0);
}
static INLINE void mainloop() {
  const auto millis = sys::millis()*game::speed/100.f;
  if (millis-game::lastmillis > 200.f) game::lastmillis = millis-200.f;
  else if (millis-game::lastmillis < 1.f) game::lastmillis = millis-1.f;
  static float fps = 30.0f;
  fps = (1000.0f/game::curtime+fps*50.f)/51.f;
  SDL_GL_SwapBuffers();
  // ogl::drawframe(scr_w, scr_h, fps);
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
} /* namespace sys */
} /* namespace q */

int main(int argc, const char *argv[]) {
  q::sys::start(argc, argv);
  for (;;) q::sys::mainloop();
  return 1;
}

