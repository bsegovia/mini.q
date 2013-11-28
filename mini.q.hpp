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
  sprintfmt_s(char *str) : d(str) {}
  void operator()(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vsnprintf(d, MAXDEFSTR, fmt, v);
    va_end(v);
    d[MAXDEFSTR-1] = 0;
  }
  char *d;
};
#define sprintf_s(d) sprintfmt_s d##__LINE__(d); d##__LINE__
#define sprintf_sd(d) string d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) string d; {va_list ap; va_start(ap,last); strfmt_s(d,fmt,ap); va_end(ap);}
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)

// simple min/max that can convert
template <class T, class U> INLINE T min(T t, U u) {return t<T(u)?t:T(u);}
template <class T, class U> INLINE T max(T t, U u) {return t>T(u)?t:T(u);}

// very simple fixed size circular buffer
template <typename T, u32 max_n> struct circular_buffer {
  INLINE circular_buffer() : first(0x7fffffff), n(0) {}
  INLINE T &append() { n=max(n+1,max_n); return items[(first+n-1)%max_n]; }
  INLINE T &prepend() { n=max(n+1,max_n); return items[--first%max_n]; }
  INLINE T &operator[](u32 idx) { return items[(first+idx)%max_n]; }
  T items[max_n];
  u32 first, n;
};

// very simple fixed size linear allocator
template <u32 size, u32 align=sizeof(int)> struct linear_allocator {
  INLINE linear_allocator() : head(data) {}
  INLINE char *newstring(const char *s, size_t l = 0) {
    if (l==0) l=strlen(s);
    return strn0cpy(alloc(l+1),s,int(l));
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
  INLINE T *begin() { return items; }
  INLINE T *end() { return items+n; }
  INLINE T *access(const char *key, const T *value = NULL) {
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
bool cmd(const char *n, const char *proto, cb fun);
void execute(const char *str, int isdown=1);

// command with custom name
#define CMDN(name, fun, proto) \
  static bool __##fun = q::script::cmd(#name, (q::script::cb) fun, proto)

// command with automatic name
#define CMD(name, proto) CMD(name, name, proto)

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
  auto name = q::script::ivar(#name, min, cur, max, &name, var_##name, true);

// int non-persistent variable with code to run when changed
#define IVARF(name, min, cur, max, body) \
  void var_##name() { body; } \
  auto name = q::script::ivar(#name, min, cur, max, &name, var_##name, false);

} /* namespace script */
} /* namespace q*/

