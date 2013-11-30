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
bool cmd(const char *n, cb fun, const char *proto) {
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
  if (p-w==0) return c.newstring("");
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
  const int MAXWORDS = 32;
  char *w[MAXWORDS];

  // for each ; separated statement
  for (bool cont = true; cont;) {
    auto numargs = MAXWORDS;
    c.rewind();
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
      break;
      case IVAR: VAR(int, i); break;
      case FVAR: VAR(float, f); break;
    }
  }
}
#undef VAR

// tab-completion of all idents
static int completesize = 0, completeidx = 0;
void resetcomplete(void) { completesize = 0; }

void complete(string &s) {
  if (*s!='/') sprintf_s(s)("/%s",s);
  if (!s[1]) return;
  if (!completesize) { completesize = int(strlen(s)-1); completeidx = 0; }
  int idx = 0;
  for (auto it = idents.begin(); it != idents.end(); ++it)
    if (strncmp(it->first, s+1, completesize)==0 && idx++==completeidx)
      sprintf_s(s)("/%s", it->first);
  completeidx++;
  if (completeidx>=idx) completeidx = 0;
}

bool execfile(const char *cfgfile) {
  sprintf_sd(s)("%s", cfgfile);
  char *buf = sys::loadfile(sys::path(s), NULL);
  if (!buf) return false;
  execute(buf);
  free(buf);
  return true;
}

void exec(const char *cfgfile) {
  if (!execfile(cfgfile)) con::out("could not read \"%s\"", cfgfile);
}

void writecfg(void) {
  auto f = fopen("config.q", "w");
  if (!f) return;
  fprintf(f, "// automatically written on exit, do not modify\n"
             "// delete this file to have defaults.cfg overwrite these settings\n"
             "// modify settings in game, or put settings in autoexec.cfg to override anything\n\n");
  for (auto it = idents.begin(); it != idents.end(); ++it)
    if (it->second.type == IVAR && it->second.ivar.persist)
      fprintf(f, "%s %d\n", it->first, *it->second.ivar.storage);
    else if (it->second.type == FVAR && it->second.fvar.persist)
      fprintf(f, "%s %f\n", it->first, *it->second.fvar.storage);
  fclose(f);
}

CMD(writecfg, "");

void execute(const char *str, int isdown) {ctx c; execute(c,str,isdown);}
} /* script */

/*-------------------------------------------------------------------------
 - console
 -------------------------------------------------------------------------*/
namespace con {

static ringbuffer<pair<string,float>, 128> buffer;

static void line(const char *sf, bool highlight) {
  auto &cl = buffer.prepend();
  cl.second = game::lastmillis;
  if (highlight) sprintf_s(cl.first)("\f%s", sf);
  else strcpy_s(cl.first, sf);
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

// keymap is defined externally in keymap.q
struct keym { int code; const char *name; string action; } keyms[256];
static int numkm = 0;
static void keymap(const char *code, const char *key, const char *action) {
  static linear_allocator<4096,sizeof(char)> lin;
  keyms[numkm].code = atoi(code);
  keyms[numkm].name = lin.newstring(key);
  sprintf_s(keyms[numkm++].action)("%s", action);
}
CMD(keymap, "sss");

static void bindkey(char *key, char *action) {
  for (char *x = key; *x; x++) *x = toupper(*x);
  loopi(numkm) if (strcmp(keyms[i].name, key)==0) {
    strcpy_s(keyms[i].action, action);
    return;
  }
  con::out("unknown key \"%s\"", key);
}
CMDN(bind, bindkey, "ss");

static const int ndraw = 5;
static int conskip = 0;
static bool saycommandon = false;
static string cmdbuf;

static void setconskip(const int &n) {
  conskip += n;
  if (conskip < 0) conskip = 0;
}
CMDN(conskip, setconskip, "i");

static void saycommand(const char *init) {
  SDL_EnableUNICODE(saycommandon = (init!=NULL));
  /* if (!editmode) */ sys::keyrepeat(saycommandon);
  if (!init) init = "";
  strcpy_s(cmdbuf, init);
}
CMD(saycommand, "s");

void keypress(int code, bool isdown, int cooked) {
  static ringbuffer<string, 64> h;
  static int hpos = 0;
  if (saycommandon) { // keystrokes go to commandline
    if (isdown) {
      switch (code) {
        case SDLK_RETURN: break;
        case SDLK_BACKSPACE:
        case SDLK_LEFT: {
          loopi(cmdbuf[i]) if (!cmdbuf[i+1]) cmdbuf[i] = 0;
          script::resetcomplete();
          break;
        }
        case SDLK_UP: if (hpos) strcpy_s(cmdbuf, h[--hpos]); break;
        case SDLK_DOWN: if (hpos<int(h.n)) strcpy_s(cmdbuf, h[hpos++]); break;
        case SDLK_TAB: script::complete(cmdbuf); break;
        default:
          script::resetcomplete();
          if (cooked) {
            const char add[] = { char(cooked), 0 };
            strcat_s(cmdbuf, add);
          }
      }
    } else {
      if (code==SDLK_RETURN) {
        if (cmdbuf[0]) {
          if (h.empty() || strcmp(h.back(), cmdbuf))
            strcpy_s(h.append(), cmdbuf);
          hpos = h.n;
          if (cmdbuf[0]=='/')
            script::execute(cmdbuf, true);
          else
            /*client::toserver(cmdbuf)*/;
        }
        saycommand(NULL);
      }
      else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else /* if (!menu::key(code, isdown))*/ { // keystrokes go to menu
    loopi(numkm) if (keyms[i].code==code) { // keystrokes go to game
      script::execute(keyms[i].action);
      return;
    }
  }
}
} /* namespace con */

/*-------------------------------------------------------------------------
 - opengl routines
 -------------------------------------------------------------------------*/
namespace ogl {
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#define OGLPROC(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */
static void *getfunction(const char *name) {
  void *ptr = SDL_GL_GetProcAddress(name);
  if (ptr == NULL) sys::fatal("opengl 2 is required");
  return ptr;
}
void start() {

#if !defined(__WEBGL__)
// on windows, we directly load OpenGL 1.1 functions
#if defined(__WIN32__)
  #define OGLPROC110(FIELD,NAME,PROTOTYPE) FIELD = (PROTOTYPE) NAME;
#else
  #define OGLPROC110 OGLPROC
#endif /* __WIN32__ */
#define OGLPROC(FIELD,NAME,PROTO) FIELD = (PROTO) getfunction(#NAME);
#include "ogl.hxx"
#undef OGLPROC110
#undef OGLPROC
#endif /* __WEBGL__ */
}
void end() { }
#if 0
/*----------------------------*/
/* very simple state tracking */
/*----------------------------*/
union {
  struct {
    u32 shader:1; // will force to reload everything
    u32 mvp:1;
    u32 fog:1;
    u32 overbright:1;
  } flags;
  u32 any;
} dirty;
static u32 bindedvbo[BUFFER_NUM];
static u32 enabledattribarray[ATTRIB_NUM];
static struct shader *bindedshader = NULL;

static const u32 TEX_NUM = 8;
static u32 bindedtexture[TEX_NUM];

void enableattribarray(u32 target) {
  if (!enabledattribarray[target]) {
    enabledattribarray[target] = 1;
    OGL(EnableVertexAttribArray, target);
  }
}

void disableattribarray(u32 target) {
  if (enabledattribarray[target]) {
    enabledattribarray[target] = 0;
    OGL(DisableVertexAttribArray, target);
  }
}

/*----------------------------*/
/* simple resource management */
/*----------------------------*/
static s32 texturenum = 0, buffernum = 0, programnum = 0;

void gentextures(s32 n, u32 *id) {
  texturenum += n;
  OGL(GenTextures, n, id);
}
void deletetextures(s32 n, u32 *id) {
  texturenum -= n;
  if (texturenum < 0) fatal("textures already freed");
  OGL(DeleteTextures, n, id);
}
void genbuffers(s32 n, u32 *id) {
  buffernum += n;
  OGL(GenBuffers, n, id);
}
void deletebuffers(s32 n, u32 *id) {
  buffernum -= n;
  if (buffernum < 0) fatal("buffers already freed");
  OGL(DeleteBuffers, n, id);
}

static u32 createprogram(void) {
  programnum++;
#if defined(__WEBGL__)
  return glCreateProgram();
#else
  return CreateProgram();
#endif // __WEBGL__
}
static void deleteprogram(u32 id) {
  programnum--;
  if (programnum < 0) fatal("program already freed");
  OGL(DeleteProgram, id);
}

/*-----------------------------------*/
/* immediate mode and buffer support */
/*-----------------------------------*/
static const u32 glbufferbinding[BUFFER_NUM] = {
  GL_ARRAY_BUFFER,
  GL_ELEMENT_ARRAY_BUFFER
};
void bindbuffer(u32 target, u32 buffer) {
  if (bindedvbo[target] != buffer) {
    OGL(BindBuffer, glbufferbinding[target], buffer);
    bindedvbo[target] = buffer;
  }
}

// attributes in the vertex buffer
static struct {int n, type, offset;} immattribs[ATTRIB_NUM];
static int immvertexsz = 0;

// we use two big circular buffers to handle immediate mode
static int immbuffersize = 16*MB;
static int bigvbooffset=0, bigibooffset=0;
static int drawibooffset=0, drawvbooffset=0;
static u32 bigvbo=0u, bigibo=0u;

static void initbuffer(u32 &bo, int target, int size) {
  if (bo == 0u) genbuffers(1, &bo);
  bindbuffer(target, bo);
  OGL(BufferData, glbufferbinding[target], size, NULL, GL_DYNAMIC_DRAW);
  bindbuffer(target, 0);
}

static void imminit(void) {
  initbuffer(bigvbo, ARRAY_BUFFER, immbuffersize);
  initbuffer(bigibo, ELEMENT_ARRAY_BUFFER, immbuffersize);
  memset(immattribs, 0, sizeof(immattribs));
}

void immattrib(int attrib, int n, int type, int offset) {
  immattribs[attrib].n = n;
  immattribs[attrib].type = type;
  immattribs[attrib].offset = offset;
}

void immvertexsize(int sz) { immvertexsz = sz; }

static void immsetallattribs(void) {
  loopi(ATTRIB_NUM) {
    if (!enabledattribarray[i]) continue;
    const void *fake = (const void *) intptr_t(drawvbooffset+immattribs[i].offset);
    OGL(VertexAttribPointer, i, immattribs[i].n, immattribs[i].type, 0, immvertexsz, fake);
  }
}

static bool immsetdata(int target, int sz, const void *data) {
  if (sz >= immbuffersize) {
    console::out("too many immediate items to render");
    return false;
  }
  u32 &bo = target==ARRAY_BUFFER ? bigvbo : bigibo;
  int &offset = target==ARRAY_BUFFER ? bigvbooffset : bigibooffset;
  int &drawoffset = target==ARRAY_BUFFER ? drawvbooffset : drawibooffset;
  if (offset+sz > immbuffersize) {
    OGL(Flush);
    bindbuffer(target, 0);
    offset = 0u;
    bindbuffer(target, bo);
  }
  bindbuffer(target, bo);
  OGL(BufferSubData, glbufferbinding[target], offset, sz, data);
  drawoffset = offset;
  offset += sz;
  return true;
}

static bool immvertices(int sz, const void *vertices) {
  return immsetdata(ARRAY_BUFFER, sz, vertices);
}

void immdrawarrays(int mode, int count, int type) {
  drawarrays(mode,count,type);
}

void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices) {
  int indexsz = count;
  int maxindex = 0;
  switch (type) {
    case GL_UNSIGNED_INT:
      indexsz*=sizeof(u32);
      loopi(count) maxindex = max(maxindex,int(((u32*)indices)[i]));
    break;
    case GL_UNSIGNED_SHORT:
      indexsz*=sizeof(u16);
      loopi(count) maxindex = max(maxindex,int(((u16*)indices)[i]));
    break;
    case GL_UNSIGNED_BYTE:
      indexsz*=sizeof(u8);
      loopi(count) maxindex = max(maxindex,int(((u8*)indices)[i]));
    break;
  };
  if (!immsetdata(ELEMENT_ARRAY_BUFFER, indexsz, indices)) return;
  if (!immvertices((maxindex+1)*immvertexsz, vertices)) return;
  immsetallattribs();
  const void *fake = (const void *) intptr_t(drawibooffset);
  drawelements(mode, count, type, fake);
}

void immdraw(int mode, int pos, int tex, int col, size_t n, const float *data) {
  const int sz = (pos+tex+col)*sizeof(float);
  if (!immvertices(n*sz, data)) return;
  loopi(ATTRIB_NUM) disableattribarray(i);
  if (pos) {
    immattrib(ogl::POS0, pos, GL_FLOAT, (tex+col)*sizeof(float));
    enableattribarray(POS0);
  } else
    disableattribarray(POS0);
  if (tex) {
    immattrib(ogl::TEX0, tex, GL_FLOAT, col*sizeof(float));
    enableattribarray(TEX0);
  } else
    disableattribarray(TEX0);
  if (col) {
    immattrib(ogl::COL, col, GL_FLOAT, 0);
    enableattribarray(COL);
  } else
    disableattribarray(COL);
  immvertexsize(sz);
  immsetallattribs();
  immdrawarrays(mode, 0, n);
}
#endif
} /* namespace ogl */

/*-------------------------------------------------------------------------
 - main entry / exit points
 -------------------------------------------------------------------------*/
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
  QueryPerformanceCounter(&val);
  return float(val.QuadPart) / float(freq.QuadPart) * 1e3f;
}
#else
float millis() {
  struct timeval tp; gettimeofday(&tp,NULL);
  return float(tp.tv_sec) * 1e3f + float(tp.tv_usec)/1e3f;
}
#endif

static int scr_w = 800, scr_h = 600, grabmouse = 0;

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

  con::out("init: video: ogl");
  ogl::start();

  con::out("init: video: misc");
  SDL_WM_SetCaption("mini.q", NULL);
  sys::keyrepeat(true);
  SDL_ShowCursor(0);
  script::exec("data/keymap.q");
}

static INLINE void mainloop() {
  const auto millis = sys::millis()*game::speed/100.f;
  if (millis-game::lastmillis > 200.f) game::lastmillis = millis-200.f;
  else if (millis-game::lastmillis < 1.f) game::lastmillis = millis-1.f;
  static float fps = 30.0f;
  fps = (1000.0f/game::curtime+fps*50.f)/51.f;
  SDL_GL_SwapBuffers();
  OGL(ClearColor, 1.f, 1.f, 1.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

