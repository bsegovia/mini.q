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
 - intrinsics
 -------------------------------------------------------------------------*/
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__SSE3__)
#include <pmmintrin.h>
#endif

#if defined(__SSSE3__)
#include <tmmintrin.h>
#endif

#if defined (__SSE4_1__)
#include <smmintrin.h>
#endif

#if defined (__SSE4_2__)
#include <nmmintrin.h>
#endif

#if defined(__AVX__) || defined(__MIC__)
#include <immintrin.h>
#endif

#if defined(__BMI__) && defined(__GNUC__)
#define _tzcnt_u32 __tzcnt_u32
#define _tzcnt_u64 __tzcnt_u64
#endif

#if defined(__LZCNT__)
#define _lzcnt_u32 __lzcnt32
#define _lzcnt_u64 __lzcnt64
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
#define vsnprintf      _vsnprintf
#define snprintf       _snprintf
#define MAYBE_UNUSED
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
#define CACHE_LINE_ALIGNMENT 64
#define CACHE_LINE_ALIGNED ALIGNED(CACHE_LINE_ALIGNMENT)
#define DEFAULT_ALIGNMENT CACHE_LINE_ALIGNMENT
#define DEFAULT_ALIGNED ALIGNED(DEFAULT_ALIGNMENT)

#define OFFSETOF(STR,F) (uintptr_t(&((STR*)0)->F))
#define ALIGN(X,A) (((X) % (A)) ? ((X) + (A) - ((X) % (A))) : (X))
#define rnd(max) (rand()%(max))
#define rndreset() (srand(1))
#define rndtime() {loopi(int(lastmillis)&0xf) rnd(i+1);}
#define loop(v,m) for (int v = 0; v < int(m); ++v)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define loopl(m) loop(l,m)
#define loopx(m) loop(x,m)
#define loopy(m) loop(y,m)
#define loopz(m) loop(z,m)
#define loopv(v)    for (int i = 0; i<(v).length(); ++i)
#define loopvj(v)    for (int j = 0; j<(v).length(); ++j)
#define looprev(n) for (int i = n-1; i>=0; --i)
#define loopvrev(v) for (int i = (v).length()-1; i>=0; --i)
#define range(v,m,M) for (int v = int(m); v < int(M); ++v)
#define rangei(m,M) range(i,m,M)
#define rangej(m,M) range(j,m,M)
#define rangek(m,M) range(k,m,M)
#define loopxy(org, end, Z)\
  for (int Y = int((org).y); Y < int((end).y); ++Y)\
  for (auto xyz = vec3i((org).x,Y,Z); xyz.x < int((end).x); ++xyz.x)
#define loopxyz(org, end) \
  for (int Z = int((org).z); Z < int((end).z); ++Z)\
  for (int Y = int((org).y); Y < int((end).y); ++Y)\
  for (vec3i xyz((org).x,Y,Z); xyz.x < int((end).x); ++xyz.x)
#define stepxyz(org, end, step) \
  for (int Z = int((org).z); Z < int((end).z); Z += int((step).z))\
  for (int Y = int((org).y); Y < int((end).y); Y += int((step).y))\
  for (auto sxyz = vec3i((org).x,Y,Z); sxyz.x < int((end).x); sxyz.x += int((step).x))

#define ARRAY_ELEM_NUM(A) sizeof(A) / sizeof(A[0])
#define ZERO(PTR) memset(PTR, 0, sizeof(*PTR))
#define MAKE_VARIADIC(NAME)\
INLINE void NAME##v(void) {}\
template <typename First, typename... Rest>\
INLINE void NAME##v(First first, Rest... rest) {\
  NAME(first);\
  NAME##v(rest...);\
}
#define COMMA ,

// concatenation
#define JOIN(X, Y) _DO_JOIN(X, Y)
#define _DO_JOIN(X, Y) _DO_JOIN2(X, Y)
#define _DO_JOIN2(X, Y) X##Y

// stringification
#define STRINGIFY(X) _DO_STRINGIFY(X)
#define _DO_STRINGIFY(X) #X

#if !defined(RELEASE)
#define STATS(X) q::s32 X = 0;
#define STATS_ADD(X,Y) (atomic_add(&X,Y))
#define STATS_INC(X) (atomic_add(&X,1))
#define STATS_OUT(X) { printf(#X": %.0lf\n", double(X)); }
#define STATS_RATIO(X,Y) printf(#X": %.2lf%% (" #Y ")\n", double(X)/double(Y)*100.0)
#else
#define STATS(X)
#define STATS_ADD(X,Y)
#define STATS_INC(X)
#define STATS_OUT(X)
#define STATS_RATIO(X,Y)
#endif

// global variable setter / getter
#define GLOBAL_VAR(NAME,VARNAME,TYPE)\
TYPE NAME(void) { return VARNAME; }\
void set##NAME(TYPE x) { VARNAME = x; }

#define GLOBAL_VAR_DECL(NAME,TYPE)\
extern TYPE NAME(void);\
extern void set##NAME(TYPE x);

/*-------------------------------------------------------------------------
 - standard types and some simple type traits
 -------------------------------------------------------------------------*/
namespace q {
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
template <typename T, typename U> struct typeequal { enum {value = false}; };
template <typename T> struct typeequal<T,T> { enum {value = true}; };

} /* namespace q */

/*-------------------------------------------------------------------------
 - windows intrinsics
 -------------------------------------------------------------------------*/
#if defined(__WIN32__)
#include <intrin.h>

INLINE q::u64 __rdpmc(int i) {
  return __readpmc(i);
}

INLINE int __popcnt(int in) {
  return _mm_popcnt_u32(in);
}

#if !defined(_MSC_VER)
INLINE unsigned int __popcnt(unsigned int in) {
  return _mm_popcnt_u32(in);
}
#endif

#if defined(__X86_64__)
INLINE long long __popcnt(long long in) {
  return _mm_popcnt_u64(in);
}
INLINE size_t __popcnt(size_t in) {
  return _mm_popcnt_u64(in);
}
#endif

INLINE int __bsf(int v) {
#if defined(__AVX2__)
  return _tzcnt_u32(v);
#else
  unsigned long r = 0; _BitScanForward(&r,v); return r;
#endif
}

INLINE unsigned int __bsf(unsigned int v) {
#if defined(__AVX2__)
  return _tzcnt_u32(v);
#else
  unsigned long r = 0; _BitScanForward(&r,v); return r;
#endif
}

INLINE int __bsr(int v) {
  unsigned long r = 0; _BitScanReverse(&r,v); return r;
}

INLINE int __btc(int v, int i) {
  long r = v; _bittestandcomplement(&r,i); return r;
}

INLINE int __bts(int v, int i) {
  long r = v; _bittestandset(&r,i); return r;
}

INLINE int __btr(int v, int i) {
  long r = v; _bittestandreset(&r,i); return r;
}

INLINE int bitscan(int v) {
#if defined(__AVX2__)
  return _tzcnt_u32(v);
#else
  return __bsf(v);
#endif
}

INLINE int clz(const int x) {
#if defined(__AVX2__)
  return _lzcnt_u32(x);
#else
  if (unlikely(x == 0)) return 32;
  return 31 - __bsr(x);
#endif
}

INLINE int __bscf(int& v) {
  int i = __bsf(v);
  v &= v-1;
  return i;
}

INLINE unsigned int __bscf(unsigned int& v) {
  unsigned int i = __bsf(v);
  v &= v-1;
  return i;
}

#if defined(__X86_64__)

INLINE size_t __bsf(size_t v) {
#if defined(__AVX2__)
  return _tzcnt_u64(v);
#else
  unsigned long r = 0; _BitScanForward64(&r,v); return r;
#endif
}

INLINE size_t __bsr(size_t v) {
  unsigned long r = 0; _BitScanReverse64(&r,v); return r;
}

INLINE size_t __btc(size_t v, size_t i) {
  size_t r = v; _bittestandcomplement64((__int64*)&r,i); return r;
}

INLINE size_t __bts(size_t v, size_t i) {
  __int64 r = v; _bittestandset64(&r,i); return r;
}

INLINE size_t __btr(size_t v, size_t i) {
  __int64 r = v; _bittestandreset64(&r,i); return r;
}

INLINE size_t bitscan(size_t v) {
#if defined(__AVX2__)
#if defined(__X86_64__)
  return _tzcnt_u64(v);
#else
  return _tzcnt_u32(v);
#endif
#else
  return __bsf(v);
#endif
}

INLINE size_t __bscf(size_t& v) {
  size_t i = __bsf(v);
  v &= v-1;
  return i;
}
#endif
#endif

/*-------------------------------------------------------------------------
 - unix intrinsics
 -------------------------------------------------------------------------*/
#if !defined(__WIN32__)
#if defined(__i386__) && defined(__PIC__)

INLINE void __cpuid(int out[4], int op) {
  asm volatile ("xchg{l}\t{%%}ebx, %1\n\t"
                "cpuid\n\t"
                "xchg{l}\t{%%}ebx, %1\n\t"
                : "=a"(out[0]), "=r"(out[1]), "=c"(out[2]), "=d"(out[3])
                : "0"(op));
}

INLINE void __cpuid_count(int out[4], int op1, int op2) {
  asm volatile ("xchg{l}\t{%%}ebx, %1\n\t"
                "cpuid\n\t"
                "xchg{l}\t{%%}ebx, %1\n\t"
                : "=a" (out[0]), "=r" (out[1]), "=c" (out[2]), "=d" (out[3])
                : "0" (op1), "2" (op2));
}

#else

INLINE void __cpuid(int out[4], int op) {
  asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op));
}

INLINE void __cpuid_count(int out[4], int op1, int op2) {
  asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op1), "c"(op2));
}

#endif

INLINE q::u64 __rdtsc()  {
  q::u32 high,low;
  asm volatile ("rdtsc" : "=d"(high), "=a"(low));
  return (((q::u64)high) << 32) + (q::u64)low;
}

INLINE q::u64 __rdpmc(int i) {
  q::u32 high,low;
  asm volatile ("rdpmc" : "=d"(high), "=a"(low) : "c"(i));
  return (((q::u64)high) << 32) + (q::u64)low;
}

INLINE unsigned int __popcnt(unsigned int in) {
  int r = 0; asm ("popcnt %1,%0" : "=r"(r) : "r"(in)); return r;
}

INLINE int __bsf(int v) {
  int r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE int __bsr(int v) {
  int r = 0; asm ("bsr %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE int __btc(int v, int i) {
  int r = 0; asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags" ); return r;
}

INLINE int __bts(int v, int i) {
  int r = 0; asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE int __btr(int v, int i) {
  int r = 0; asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE size_t __bsf(size_t v) {
  size_t r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE unsigned int __bsf(unsigned int v) {
  unsigned int r = 0; asm ("bsf %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE size_t __bsr(size_t v) {
  size_t r = 0; asm ("bsr %1,%0" : "=r"(r) : "r"(v)); return r;
}

INLINE size_t __btc(size_t v, size_t i) {
  size_t r = 0; asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags" ); return r;
}

INLINE size_t __bts(size_t v, size_t i) {
  size_t r = 0; asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE size_t __btr(size_t v, size_t i) {
  size_t r = 0; asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); return r;
}

INLINE int bitscan(int v) {
#if defined(__AVX2__)
  return _tzcnt_u32(v);
#else
  return __bsf(v);
#endif
}

INLINE unsigned int bitscan(unsigned int v) {
#if defined(__AVX2__)
  return _tzcnt_u32(v);
#else
  return __bsf(v);
#endif
}

INLINE size_t bitscan(size_t v) {
#if defined(__AVX2__)
#if defined(__X86_64__)
  return _tzcnt_u64(v);
#else
  return _tzcnt_u32(v);
#endif
#else
  return __bsf(v);
#endif
}

INLINE int clz(const int x)
{
#if defined(__AVX2__)
  return _lzcnt_u32(x);
#else
  if (x == 0) return 32;
  return 31 - __bsr(x);
#endif
}

INLINE int __bscf(int& v)
{
  int i = bitscan(v);
#if defined(__AVX2__)
  v &= v-1;
#else
  v = __btc(v,i);
#endif
  return i;
}

INLINE unsigned int __bscf(unsigned int& v) {
  unsigned int i = bitscan(v);
  v &= v-1;
  return i;
}

INLINE size_t __bscf(size_t& v) {
  size_t i = bitscan(v);
#if defined(__AVX2__)
  v &= v-1;
#else
  v = __btc(v,i);
#endif
  return i;
}
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
#include <intrin.h>
#pragma warning (disable:4200) // no standard zero sized array
#undef min
#undef max
#else
#include <sys/time.h>
#endif

#if defined(__EMSCRIPTEN__)
#include "GLES2/gl2.h"
#else
#if defined(__WIN32__)
#define GL3_PROTOTYPES
#include "GL/gl3.h"
#endif
#endif

namespace q {
template <u32 sz> struct ptrtype {};
template <> struct ptrtype<4> { typedef u32 type; };
template <> struct ptrtype<8> { typedef u64 type; };
typedef typename ptrtype<sizeof(void*)>::type uintptr;
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
void writebmp(const int *data, int w, int h, const char *filename);

/*-------------------------------------------------------------------------
 - memory debugging / tracking facilities
 -------------------------------------------------------------------------*/
void memstart();
void *memalloc(size_t sz, const char *filename, int linenum);
void *memrealloc(void *ptr, size_t sz, const char *filename, int linenum);
void memfree(void *);
void *memalignedalloc(size_t size, size_t align, const char *file, int lineno);
void memalignedfree(const void* ptr);
template <typename T> void callctor(void *ptr) { new (ptr) T; }
template <typename T, typename... Args>
INLINE void callctor(void *ptr, Args&&... args) { new (ptr) T(args...); }

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

#define ALIGNEDMALLOC(SZ,ALIGN) q::sys::memalignedalloc(SZ, ALIGN, __FILE__, __LINE__)
#define ALIGNEDFREE(PTR) q::sys::memalignedfree(PTR)
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

