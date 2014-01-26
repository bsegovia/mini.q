/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sys.hpp -> exposes platform specific code
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
#include <new>
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
#define COMPILER_WRITE_BARRIER       _WriteBarrier()
#define COMPILER_READ_WRITE_BARRIER  _ReadWriteBarrier()
#define CDECL           __cdecl
#define vsnprintf       _vsnprintf
#define MAYBE_UNUSED    __attribute__((used))
#define MAYALIAS
#define PATHDIV         '\\'
#if _MSC_VER >= 1400
#pragma intrinsic(_ReadBarrier)
#define COMPILER_READ_BARRIER        _ReadBarrier()
#else
#define COMPILER_READ_BARRIER        _ReadWriteBarrier()
#endif
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
#define MAYBE_UNUSED
#define COMPILER_READ_WRITE_BARRIER asm volatile("" ::: "memory");
#define COMPILER_WRITE_BARRIER COMPILER_READ_WRITE_BARRIER
#define COMPILER_READ_BARRIER COMPILER_READ_WRITE_BARRIER
#define MAYALIAS __attribute__((__may_alias__))
#endif // __MSVC__

// alignment rules
#define DEFAULT_ALIGNMENT 16
#define DEFAULT_ALIGNED ALIGNED(DEFAULT_ALIGNMENT)
#define CACHE_LINE_ALIGNMENT 64
#define CACHE_LINE_ALIGNED ALIGNED(CACHE_LINE_ALIGNMENT)

#define OFFSETOF(STR,F) (uintptr_t(&((STR*)0)->F))
#define ALIGN(X,A) (((X) % (A)) ? ((X) + (A) - ((X) % (A))) : (X))
#define rnd(max) (rand()%(max))
#define rndreset() (srand(1))
#define rndtime() {loopi(int(lastmillis)&0xf) rnd(i+1);}
#define loop(v,m) for (int v = 0; v < int(m); ++v)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define loopx(m) loop(x,m)
#define loopy(m) loop(y,m)
#define loopz(m) loop(z,m)
#define loopv(v)    for (int i = 0; i<(v).length(); ++i)
#define loopvrev(v) for (int i = (v).length()-1; i>=0; --i)
#define range(v,m,M) for (int v = int(m); v < int(M); ++v)
#define rangei(m,M) range(i,m,M)
#define rangej(m,M) range(j,m,M)
#define rangek(m,M) range(k,m,M)
#define loopxy(org, end, Z)\
  for (int Y = int(org.y); Y < int(end.y); ++Y)\
  for (auto xyz = vec3i(org.x,Y,Z); xyz.x < int(end.x); ++xyz.x)
#define loopxyz(org, end) \
  for (int Z = int(org.z); Z < int(end.z); ++Z)\
  for (int Y = int(org.y); Y < int(end.y); ++Y)\
  for (auto xyz = vec3i(org.x,Y,Z); xyz.x < int(end.x); ++xyz.x)

#define ARRAY_ELEM_NUM(A) sizeof(A) / sizeof(A[0])
#define ZERO(PTR, SIZE) memset(PTR, 0, sizeof(SIZE))
#define MAKE_VARIADIC(NAME)\
INLINE void NAME##v(void) {}\
template <typename First, typename... Rest>\
INLINE void NAME##v(First first, Rest... rest) {\
  NAME(first);\
  NAME##v(rest...);\
}
#define COMMA ,

#if !defined(NDEBUG)
#define STATS(X) q::u64 X = 0;
#define STATS_ADD(X,Y) (X += Y)
#define STATS_INC(X) (++X)
#define STATS_OUT(X) { printf(#X": %.0lf\n", double(X)); }
#define STATS_RATIO(X,Y) printf(#X": %.2lf%%\n", double(X)/double(Y)*100.0)
#else
#define STATS(X)
#define STATS_ADD(X,Y)
#define STATS_INC(X)
#define STATS_OUT(X)
#define STATS_RATIO(X,Y)
#endif

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
static struct niltype {niltype(){}} nil MAYBE_UNUSED;

namespace sys {

/*-------------------------------------------------------------------------
 - low-level system functions
 -------------------------------------------------------------------------*/
extern int scrw, scrh;
void fatal(const char *s, const char *o = "");
void quit(const char *msg = NULL);
void keyrepeat(bool on);
float millis();
char *path(char *s);
char *loadfile(const char *fn, int *size=NULL);
void initendiancheck();
int islittleendian();
void endianswap(void *memory, int stride, int length);
u32 threadnumber();

/*-------------------------------------------------------------------------
 - memory debugging / tracking facilities
 -------------------------------------------------------------------------*/
void memstart();
void *memalloc(size_t sz, const char *filename, int linenum);
void *memrealloc(void *ptr, size_t sz, const char *filename, int linenum);
void memfree(void *);
template <typename T> void callctor(void *ptr) { new (ptr) T; }
template <typename T, typename... Args>
void callctor(void *ptr, Args&&... args) { new (ptr) T(args...); }

template <typename T, typename... Args>
INLINE T *memconstructa(s32 n, const char *filename, int linenum, Args&&... args) {
  void *ptr = (void*) memalloc(n*sizeof(T)+DEFAULT_ALIGNMENT, filename, linenum);
  *(s32*)ptr = n;
  T *array = (T*)((char*)ptr+DEFAULT_ALIGNMENT);
  loopi(n) callctor<T>(array+i, args...);
  return array;
}

template <typename T, typename... Args>
INLINE T *memconstruct(const char *filename, int linenum, Args&&... args) {
  T *ptr = (T*) memalloc(sizeof(T), filename, linenum);
  callctor<T>(ptr, args...);
  return ptr;
}
template <typename T>
INLINE T *memconstruct(const char *filename, int linenum) {
  T *ptr = (T*) memalloc(sizeof(T), filename, linenum);
  new (ptr) T;
  return ptr;
}
template <typename T> INLINE void memdestroy(T *ptr) {
  ptr->~T();
  memfree(ptr);
}
template <typename T> INLINE void memdestroya(T *array) {
  s32 *ptr = (s32*) ((char*)array-DEFAULT_ALIGNMENT);
  loopi(*ptr) array[i].~T();
  memfree(ptr);
}
} /* namespace sys */

#define MALLOC(SZ) q::sys::memalloc(SZ, __FILE__, __LINE__)
#define REALLOC(PTR, SZ) q::sys::memrealloc(PTR, SZ, __FILE__, __LINE__)
#define FREE(PTR) q::sys::memfree(PTR)
#define NEWE(X) q::sys::memconstruct<X>(__FILE__,__LINE__)
#define NEWAE(X,N) q::sys::memconstructa<X>(N,__FILE__,__LINE__)
#define NEW(X,...) q::sys::memconstruct<X>(__FILE__,__LINE__,__VA_ARGS__)
#define NEWA(X,N,...) q::sys::memconstructa<X>(N,__FILE__,__LINE__,__VA_ARGS__)
#define DEL(X) do { q::sys::memdestroy(X); } while (0)
#define DELA(X) do { q::sys::memdestroya(X); } while (0)
#define SAFE_DEL(X) do { if (X) DEL(X); } while (0)
#define SAFE_DELA(X) do { if (X) DELA(X); X = NULL; } while (0)

/*-------------------------------------------------------------------------
 - atomics / barriers / locks
 -------------------------------------------------------------------------*/
#if defined(__MSVC__)
INLINE s32 atomic_add(volatile s32* m, const s32 v) {
  return _InterlockedExchangeAdd((volatile long*)m,v);
}
INLINE s32 atomic_cmpxchg(volatile s32* m, const s32 v, const s32 c) {
  return _InterlockedCompareExchange((volatile long*)m,v,c);
}
#elif defined(__JAVASCRIPT__)
INLINE s32 atomic_add(s32 volatile* value, s32 input) {
  const s32 initial = value;
  *value += input;
  return initial;
}
INLINE s32 atomic_cmpxchg(volatile s32* m, const s32 v, const s32 c) {
  const s32 initial = *m;
  if (*m == c) *m = v;
  return initial;
}
#else
INLINE s32 atomic_add(s32 volatile* value, s32 input) {
  asm volatile("lock xadd %0,%1" : "+r"(input), "+m"(*value) : "r"(input), "m"(*value));
  return input;
}

INLINE s32 atomic_cmpxchg(s32 volatile* value, const s32 input, s32 comparand) {
  asm volatile("lock cmpxchg %2,%0" : "=m"(*value), "=a"(comparand) : "r"(input), "m"(*value), "a"(comparand) : "flags");
  return comparand;
}
#endif // __MSVC__

#if defined(__X86__) || defined(__X86_64__) || defined(__JAVASCRIPT__)
template <typename T>
INLINE T loadacquire(volatile T *ptr) {
  COMPILER_READ_WRITE_BARRIER;
  T x = *ptr;
  COMPILER_READ_WRITE_BARRIER;
  return x;
}
template <typename T>
INLINE void storerelease(volatile T *ptr, T x) {
  COMPILER_READ_WRITE_BARRIER;
  *ptr = x;
  COMPILER_READ_WRITE_BARRIER;
}
#else
#error "unknown platform"
#endif
} /* namespace q */

