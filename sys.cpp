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
static int islittleendian_ = 1;
void initendiancheck(void) { islittleendian_ = *((char*)&islittleendian_); }
int islittleendian(void) { return islittleendian_; }

#if defined(MEMORY_DEBUGGER)
struct DEFAULT_ALIGNED memblock : intrusive_list_node {
  INLINE memblock(size_t sz, const char *file, int linenum) :
    file(file), linenum(linenum), allocnum(0), size(sz)
  {rbound() = lbound() = 0xdeadc0de;}
  const char *file;
  u32 linenum, allocnum, size;
  u32 bound;
  INLINE u32 &rbound(void) {return (&bound)[size/sizeof(u32)+1];}
  INLINE u32 &lbound(void) {return bound;}
};

static intrusive_list<memblock> *memlist = NULL;
static SDL_mutex *memmutex = NULL;
static u32 memallocnum = 0;
static bool memfirstalloc = true;

static void memlinkblock(memblock *node) {
  if (memmutex) SDL_LockMutex(memmutex);
  node->allocnum = memallocnum++;
  // if (node->allocnum == 0) DEBUGBREAK;
  memlist->push_back(node);
  if (memmutex) SDL_UnlockMutex(memmutex);
}
static void memunlinkblock(memblock *node) {
  if (memmutex) SDL_LockMutex(memmutex);
  unlink(node);
h if (memmutex) SDL_UnlockMutex(memmutex);
}h

#define MEMOUT(S,B)\
  sprintf_sd(S)("file: %s, line %i, size %i bytes, alloc %i",\
                (B)->file, (B)->linenum, (B)->size, (B)->allocnum)
static void memcheckbounds(memblock *node) {
  if (node->lbound() != 0xdeadc0de || node->rbound() != 0xdeadc0de) {
    fprintf(stderr, "memory corruption detected (alloc %i)\n", node->allocnum);
    DEBUGBREAK;
    _exit(EXIT_FAILURE);
  }
}
static void memoutputalloc(void) {
  size_t sz = 0;
  if (memlist != NULL) {
    for (auto it = memlist->begin(); it != memlist->end(); ++it) {
      MEMOUT(unfreed, it);
      fprintf(stderr, "unfreed allocation: %s\n", unfreed);
      sz += it->size;
    }
    if (sz > 0) fprintf(stderr, "total unfreed: %fKB \n", float(sz)/1000.f);
    delete memlist; 
  }
}

static void memonfirstalloc(void) {
  if (memfirstalloc) {
    memlist = new intrusive_list<memblock>();
    atexit(memoutputalloc);
    memfirstalloc = false;
  }
}
#undef MEMOUT

void meminit(void) { memmutex = SDL_CreateMutex(); }
void *memalloc(size_t sz, const char *filename, int linenum) {
  memonfirstalloc();
  if (sz) {
    sz = ALIGN(sz, DEFAULT_ALIGNMENT);
    auto ptr = malloc(sz+sizeof(memblock)+sizeof(u32));
    auto block = new (ptr) memblock(sz, filename, linenum);
    memlinkblock(block);
    return (void*) (block+1);
  } else
    return NULL;
}

void memfree(void *ptr) {
  memonfirstalloc();
  if (ptr) {
    auto block = (memblock*)((char*)ptr-sizeof(memblock));
    memcheckbounds(block);
    memunlinkblock(block);
    free(block);
  }
}

void *memrealloc(void *ptr, size_t sz, const char *filename, int linenum) {
  memonfirstalloc();
  auto block = (memblock*)((char*)ptr-sizeof(memblock));
  if (ptr) {
    memcheckbounds(block);
    memunlinkblock(block);
  }
  if (sz) {
    sz = ALIGN(sz, DEFAULT_ALIGNMENT);
    auto ptr = realloc(block, sz+sizeof(memblock)+sizeof(u32));
    block = new (ptr) memblock(sz, filename, linenum);
    memlinkblock(block);
    return (void*) (block+1);
  } else if (ptr)
    free(block);
  return NULL;
}

#else
void meminit(void) {}
void *memalloc(size_t sz, const char*, int) {return malloc(sz);}
void *memrealloc(void *ptr, size_t sz, const char *, int) {return realloc(ptr,sz);}
void memfree(void *ptr) {free(ptr);}
#endif // defined(MEMORY_DEBUGGER)

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

char *newstring(const char *s, size_t l, const char *filename, int linenum) {
  char *b = (char*) memalloc(l+1, filename, linenum);
  strncpy(b,s,l);
  b[l] = 0;
  return b;
}
char *newstring(const char *s, const char *filename, int linenum) {
  return newstring(s, strlen(s), filename, linenum);
}
char *newstringbuf(const char *s, const char *filename, int linenum) {
  return newstring(s, MAXDEFSTR-1, filename, linenum);
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
  initendiancheck();

  con::out("init: video: ogl");
  ogl::start(scrw, scrh);

  con::out("init: video: misc");
  SDL_WM_SetCaption("mini.q", NULL);
  keyrepeat(true);
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

