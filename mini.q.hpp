/*-------------------------------------------------------------------------
 - mini.q
 - a minimalistic multiplayer FPS
 -------------------------------------------------------------------------*/
#pragma once

/*-------------------------------------------------------------------------
 - cpu architecture
 -------------------------------------------------------------------------*/
#if defined(__x86_64__) || defined(__ia64__) || defined(_M_X64)
#define __X86_64__
#elif !defined(EMSCRIPTEN)
#define __X86__
#endif

/*-------------------------------------------------------------------------
 - operating system
 -------------------------------------------------------------------------*/
#if defined(linux) || defined(__linux__) || defined(__LINUX__)
#  if !defined(__LINUX__)
#     define __LINUX__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

#if defined(__FreeBSD__) || defined(__FREEBSD__)
#  if !defined(__FREEBSD__)
#     define __FREEBSD__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) && !defined(__CYGWIN__)
#  if !defined(__WIN32__)
#     define __WIN32__
#  endif
#endif

#if defined(__CYGWIN__)
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

#if defined(__APPLE__) || defined(MACOSX) || defined(__MACOSX__)
#  if !defined(__MACOSX__)
#     define __MACOSX__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

#if defined(EMSCRIPTEN)
#  define __JAVASCRIPT__
#endif

#if defined(__unix__) || defined (unix) || defined(__unix) || defined(_unix)
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

/*-------------------------------------------------------------------------
 - compiler
 -------------------------------------------------------------------------*/
#ifdef __GNUC__
// #define __GNUC__
#endif

#ifdef __INTEL_COMPILER
#define __ICC__
#endif

#ifdef _MSC_VER
#define __MSVC__
#endif

#ifdef __clang__
#define __CLANG__
#endif

#if defined(EMSCRIPTEN)
#define __EMSCRIPTEN__
#endif

/*-------------------------------------------------------------------------
 - dependencies
 -------------------------------------------------------------------------*/
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cassert>
#include <SDL/SDL.h>

#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#else
#include <sys/time.h>
#endif

#if defined(__EMSCRIPTEN__)
#include "GLES2/gl2.h"
#else
#include "GL/gl3.h"
#endif

/*-------------------------------------------------------------------------
 - useful macros
 -------------------------------------------------------------------------*/
#ifdef __MSVC__
#undef NOINLINE
#define NOINLINE        __declspec(noinline)
#define INLINE          __forceinline
#define RESTRICT        __restrict
#define THREAD          __declspec(thread)
#define ALIGNED(...)    __declspec(align(__VA_ARGS__))
//#define __FUNCTION__  __FUNCTION__
#define DEBUGBREAK      __debugbreak()
#define CDECL           __cdecl
#define vsnprintf       _vsnprintf
#define PATHDIV         '\\'
#else // __MSVC__
#undef NOINLINE
#undef INLINE
#define NOINLINE        __attribute__((noinline))
#define INLINE          inline __attribute__((always_inline))
#define RESTRICT        __restrict
#define THREAD          __thread
#define ALIGNED(...)    __attribute__((aligned(__VA_ARGS__)))
#define __FUNCTION__    __PRETTY_FUNCTION__
#define DEBUGBREAK      asm ("int $3")
#define CDECL
#define PATHDIV         '/'
#endif // __MSVC__

#define ALIGN(X,A) (((X) % (A)) ? ((X) + (A) - ((X) % (A))) : (X))
#define loop(v,m) for(int v = 0; v < int(m); ++v)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define range(v,m,M) for(auto v = m; v < (M); ++v)
#define rangei(m,M) range(i,m,M)
#define rangej(m,M) range(j,m,M)
#define rangek(m,M) range(k,m,M)
#define MAKE_VARIADIC(NAME)\
INLINE void NAME##v(void) {}\
template <typename First, typename... Rest>\
INLINE void NAME##v(First first, Rest... rest) {\
  NAME(first);\
  NAME##v(rest...);\
}

namespace q {

/*-------------------------------------------------------------------------
 - standard types
 -------------------------------------------------------------------------*/
#if defined(__MSVC__)
typedef          __int64 s64;
typedef unsigned __int64 u64;
typedef          __int32 s32;
typedef unsigned __int32 u32;
typedef          __int16 s16;
typedef unsigned __int16 u16;
typedef          __int8  s8;
typedef unsigned __int8  u8;
#else
typedef          long long s64;
typedef unsigned long long u64;
typedef                int s32;
typedef unsigned       int u32;
typedef              short s16;
typedef unsigned     short u16;
typedef               char s8;
typedef unsigned      char u8;
#endif // __MSVC__

/*-------------------------------------------------------------------------
 - low-level system function
 -------------------------------------------------------------------------*/
namespace sys {
void fatal(const char *s, const char *o = "");
void keyrepeat(bool on);
float millis();
char *path(char *s);
char *loadfile(const char *fn, int *size);
} /* namespace sys */

/*-------------------------------------------------------------------------
 - completely stripped down "stdlib"
 -------------------------------------------------------------------------*/
// more string operations
INLINE char *strn0cpy(char *d, const char *s, int m) {strncpy(d,s,m); d[m-1]=0; return d;}

// easy safe string
static const u32 MAXDEFSTR = 260, WORDWRAP = 80;
typedef char string[MAXDEFSTR];
INLINE void strcpy_s(string &d, const char *s) { strn0cpy(d,s,MAXDEFSTR); }
INLINE void strcat_s(string &d, const char *s) { size_t n = strlen(d); strn0cpy(d+n,s,MAXDEFSTR-n); }
INLINE void strfmt_s(string &d, const char *fmt, va_list v) {
  vsnprintf(d, MAXDEFSTR, fmt, v);
  d[MAXDEFSTR-1] = 0;
}

// format string boilerplate
struct sprintfmt_s {
  INLINE sprintfmt_s(string &str) : d(str) {}
  void operator()(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vsnprintf(d, MAXDEFSTR, fmt, v);
    va_end(v);
    d[MAXDEFSTR-1] = 0;
  }
  static INLINE sprintfmt_s make(string &s) { return sprintfmt_s(s); }
  string &d;
};
#define sprintf_s(d) sprintfmt_s::make(d)
#define sprintf_sd(d) string d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) string d; {va_list ap; va_start(ap,last); strfmt_s(d,fmt,ap); va_end(ap);}
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)

// simple min/max that can convert
template <class T, class U> INLINE T min(T t, U u) {return t<T(u)?t:T(u);}
template <class T, class U> INLINE T max(T t, U u) {return t>T(u)?t:T(u);}

// very simple fixed size circular buffer
template <typename T, u32 max_n> struct ringbuffer {
  INLINE ringbuffer() : first(0x7fffffff), n(0) {}
  INLINE T &append() { n=max(n+1,max_n); return items[(first+n-1)%max_n]; }
  INLINE T &prepend() { n=max(n+1,max_n); return items[--first%max_n]; }
  INLINE T &operator[](u32 idx) { return items[(first+idx)%max_n]; }
  INLINE u32 empty() const {return n==0;}
  INLINE T &back() {return items[(first+n-1)%max_n];}
  T items[max_n];
  u32 first, n;
};

// very simple fixed size linear allocator
template <u32 size, u32 align=sizeof(int)> struct linear_allocator {
  INLINE linear_allocator() : head(data) {}
  INLINE char *newstring(const char *s, u32 l=~0u) {
    if (l==~0u) l=strlen(s);
    auto d = strncpy(alloc(l+1),s,l); d[l]=0;
    return d;
  }
  INLINE void rewind() { head=data; }
  INLINE char *alloc(int sz) {
    if (head+ALIGN(sz,align)>data+size) sys::fatal("out-of-memory");
    head += ALIGN(sz,align);
    return head-sz;
  }
  char *head, data[size];
};

// very simple pair
template <typename T, typename U> struct pair {
  INLINE pair() {}
  INLINE pair(T t, U u) : first(t), second(u) {}
  T first; U second;
};
template <typename T, typename U>
INLINE pair<T,U> makepair(const T &t, const U &u){return pair<T,U>(t,u);}

// very simple fixed size hash table (linear probing)
template <typename T, u32 max_n=1024> struct hashtable {
  hashtable() : n(0) {}
  INLINE pair<const char*, T> *begin() { return items; }
  INLINE pair<const char*, T> *end() { return items+n; }
  T *access(const char *key, const T *value = NULL) {
    u32 h = 5381;
    for (u32 i = 0, k; (k = key[i]) != 0; ++i) h = ((h<<5)+h)^k;
    u32 i=0;
    for(;i<n;++i) { if (h==hashes[i] && !strcmp(key,items[i].first)) break; }
    if (value != NULL) {
      if (i>=max_n) sys::fatal("out-of-memory");
      items[i] = makepair(key,*value);
      hashes[i] = h;
      n = max(i+1,n);
    }
    return i==n ? NULL : &items[i].second;
  }
  pair<const char*, T> items[max_n];
  u32 hashes[max_n], n;
};

/*-------------------------------------------------------------------------
 - game
 -------------------------------------------------------------------------*/
namespace game {
extern float lastmillis, speed, curtime;
} /* namespace game */

/*-------------------------------------------------------------------------
 - console
 -------------------------------------------------------------------------*/
namespace con{
void out(const char *s, ...);
void keypress(int code, bool isdown, int cooked);
} /* namespace con */

/*-------------------------------------------------------------------------
 - simple script system mostly compatible with quake console
 -------------------------------------------------------------------------*/
namespace script {
typedef void (CDECL *cb)();
int ivar(const char *n, int m, int cur, int M, int *ptr, cb fun, bool persist);
float fvar(const char *n, float m, float cur, float M, float *ptr, cb fun, bool persist);
bool cmd(const char *n, cb fun, const char *proto);
void execute(const char *str, int isdown=1);

// command with custom name
#define CMDN(name, fun, proto) \
  static bool __##fun = q::script::cmd(#name, (q::script::cb) fun, proto)
// command with automatic name
#define CMD(name, proto) CMDN(name, name, proto)
// float persistent variable
#define FVARP(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, true)
// float non-persistent variable
#define FVAR(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, false)
// float persistent variable with code to run when changed
#define FVARPF(name, min, cur, max, body) \
  void var_##name() { body; } \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, true);
// float non-persistent variable with code to run when changed
#define FVARF(name, min, cur, max, body) \
  void var_##name() { body; } \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, false);
// int persistent variable
#define IVARP(name, min, cur, max) \
  auto name = q::script::ivar(#name, min, cur, max, &name, NULL, true)
// int non-persistent variable
#define IVAR(name, min, cur, max) \
  auto name = q::script::ivar(#name, min, cur, max, &name, NULL, false)
// int persistent variable with code to run when changed
#define IVARPF(name, min, cur, max, body) \
  void var_##name() { body; } \
// int non-persistent variable with code to run when changed
#define IVARF(name, min, cur, max, body) \
  void var_##name() { body; } \
  auto name = q::script::ivar(#name, min, cur, max, &name, var_##name, false);

} /* namespace script */

/*-------------------------------------------------------------------------
 - opengl interface
 -------------------------------------------------------------------------*/
namespace ogl {

// declare all GL functions
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#define OGLPROC(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */

void start();
void end();
#if 0
// vertex attributes
enum {POS0, POS1, TEX0, TEX1, TEX2, NOR, COL, ATTRIB_NUM};

// pre-allocated texture
enum {
  TEX_CROSSHAIR = 1,
  TEX_CHARACTERS,
  TEX_MARTIN_BASE,
  TEX_ITEM,
  TEX_EXPLOSION,
  TEX_MARTIN_BALL1,
  TEX_MARTIN_SMOKE,
  TEX_MARTIN_BALL2,
  TEX_MARTIN_BALL3,
  TEX_PREALLOCATED_NUM = TEX_MARTIN_BALL3
};

// quick, dirty and super simple uber-shader system
static const u32 COLOR = 0;
static const u32 FOG = 1<<0;
static const u32 KEYFRAME = 1<<1;
static const u32 DIFFUSETEX = 1<<2;
static const int subtypen = 3;
static const int shadern = 1<<subtypen;
void bindshader(u32 flags);

// track allocations
void gentextures(s32 n, u32 *id);
void genbuffers(s32 n, u32 *id);
void deletetextures(s32 n, u32 *id);
void deletebuffers(s32 n, u32 *id);

// draw helper functions
void draw(int mode, int pos, int tex, size_t n, const float *data);
void drawarrays(int mode, int first, int count);
void drawelements(int mode, int count, int type, const void *indices);
void rendermd2(const float *pos0, const float *pos1, float lerp, int n);
void drawsphere(void);

// following functions also ensure state tracking
enum {ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER, BUFFER_NUM};
void bindbuffer(u32 target, u32 buffer);
void bindgametexture(u32 target, u32 tex);
void enableattribarray(u32 target);
void disableattribarray(u32 target);

// enable /disable vertex attribs in one shot
struct setattribarray {
  INLINE setattribarray(void) {
    loopi(ATTRIB_NUM) enabled[i] = false;
  }
  template <typename First, typename... Rest>
  INLINE void operator() (First first, Rest... rest) {
    enabled[first] = true;
    operator() (rest...);
  }
  INLINE void operator()() {
    loopi(ATTRIB_NUM) if (enabled[i]) enableattribarray(i); else disableattribarray(i);
  }
  bool enabled[ATTRIB_NUM];
};

// immediate mode rendering
void immvertexsize(int sz);
void immattrib(int attrib, int n, int type, int offset);
void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices);
void immdrawarrays(int mode, int first, int count);
void immdraw(int mode, int pos, int tex, int col, size_t n, const float *data);

// matrix interface
enum {MODELVIEW, PROJECTION, MATRIX_MODE};
void matrixmode(int mode);
void identity(void);
void rotate(float angle, const vec3f &axis);
void perspective(const mat4x4f &m, float fovy, float aspect, float znear, float zfar);
void translate(const vec3f &v);
void mulmatrix(const mat4x4f &m);
void pushmatrix(void);
void popmatrix(void);
void loadmatrix(const mat4x4f &m);
void ortho(float left, float right, float bottom, float top, float znear, float zfar);
void scale(const vec3f &s);
const mat4x4f &matrix(int mode);
#endif

// OGL debug macros
#if !defined(__EMSCRIPTEN__)
#if !defined(NDEBUG)
#define OGL(NAME, ...) \
  do { \
    ogl::NAME(__VA_ARGS__); \
    if (q::ogl::GetError()) fatal("gl" #NAME " failed"); \
  } while (0)
#define OGLR(RET, NAME, ...) \
  do { \
    RET = q::ogl::NAME(__VA_ARGS__); \
    if (q::ogl::GetError()) fatal("gl" #NAME " failed"); \
  } while (0)
#else
  #define OGL(NAME, ...) do {q::ogl::NAME(__VA_ARGS__);} while(0)
  #define OGLR(RET, NAME, ...) do {RET=q::ogl::NAME(__VA_ARGS__);} while(0)
#endif /* NDEBUG */
#else /* __EMSCRIPTEN__ */
#if !defined(NDEBUG)
#define OGL(NAME, ...) \
  do { \
    gl##NAME(__VA_ARGS__); \
    if (gl##GetError()) { \
      sprintf_sd(err)("gl" #NAME " failed at line %i and file %s",__LINE__, __FILE__);\
      fatal(err); \
    } \
  } while (0)
#define OGLR(RET, NAME, ...) \
  do { \
    RET = gl##NAME(__VA_ARGS__); \
    if (gl##GetError()) fatal("gl" #NAME " failed"); \
  } while (0)
#else
  #define OGL(NAME, ...) do {gl##NAME(__VA_ARGS__);} while(0)
  #define OGLR(RET, NAME, ...) do {RET=gl##NAME(__VA_ARGS__);} while(0)
#endif /* NDEBUG */
#endif /* __EMSCRIPTEN__ */

// useful to enable / disable lot of stuff in one-liners
INLINE void enable(u32 x) { OGL(Enable,x); }
INLINE void disable(u32 x) { OGL(Disable,x); }
MAKE_VARIADIC(enable);
MAKE_VARIADIC(disable);

} /* namespace ogl */
} /* namespace q */

