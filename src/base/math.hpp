/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - math.hpp -> implements vec* and mat* routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "stl.hpp"
#include <cmath>
#include <cfloat>

namespace q {
#define TNOINLINE template <typename T> NOINLINE
#define TINLINE template <typename T> INLINE
#define UINLINE template <typename U> INLINE
#define op operator

/*-------------------------------------------------------------------------
 - useful constants
 -------------------------------------------------------------------------*/
#define CONSTANT_TYPE(TYPE,VALUE,NUM)\
static const struct TYPE {\
  INLINE TYPE(void) {}\
  INLINE op double(void) const {return double(NUM);}\
  INLINE op float (void) const {return float(NUM);}\
  INLINE op s64(void) const {return s64(NUM);}\
  INLINE op u64(void) const {return u64(NUM);}\
  INLINE op s32(void) const {return s32(NUM);}\
  INLINE op u32(void) const {return u32(NUM);}\
  INLINE op s16(void) const {return s16(NUM);}\
  INLINE op u16(void) const {return u16(NUM);}\
  INLINE op s8 (void) const {return s8 (NUM);}\
  INLINE op u8 (void) const {return u8 (NUM);}\
} VALUE MAYBE_UNUSED;
CONSTANT_TYPE(truetype,truev,true);
CONSTANT_TYPE(falsetype,falsev,false);
CONSTANT_TYPE(zerotype,zero,0);
CONSTANT_TYPE(onetype,one,1);
CONSTANT_TYPE(twotype,two,2);
CONSTANT_TYPE(pitype,pi,3.14159265358979323846);
#undef CONSTANT_TYPE

/*-------------------------------------------------------------------------
 - scalar math
 -------------------------------------------------------------------------*/
INLINE int abs    (int x)   {return std::abs(x);}
INLINE float sign (float x) {return x<0?-1.0f:1.0f;}
INLINE float rcp  (float x) {return 1.0f/x;}
INLINE float rsqrt(float x) {return 1.0f/::sqrtf(x);}
INLINE float abs  (float x) {return ::fabsf(x);}
INLINE float acos (float x) {return ::acosf (x);}
INLINE float asin (float x) {return ::asinf (x);}
INLINE float atan (float x) {return ::atanf (x);}
INLINE float cos  (float x) {return ::cosf  (x);}
INLINE float cosh (float x) {return ::coshf (x);}
INLINE float exp  (float x) {return ::expf  (x);}
INLINE float log  (float x) {return ::logf  (x);}
INLINE float log2 (float x) {return ::log(x) / 0.69314718055994530941723212145818f;}
INLINE float log10(float x) {return ::log10f(x);}
INLINE float sin  (float x) {return ::sinf  (x);}
INLINE float sinh (float x) {return ::sinhf (x);}
INLINE float sqrt (float x) {return ::sqrtf (x);}
INLINE float tan  (float x) {return ::tanf  (x);}
INLINE float tanh (float x) {return ::tanhf (x);}
INLINE float floor(float x) {return ::floorf (x);}
INLINE float ceil (float x) {return ::ceilf (x);}
INLINE float atan2(float y, float x) {return ::atan2f(y, x);}
INLINE float fmod (float x, float y) {return ::fmodf (x, y);}
INLINE float pow  (float x, float y) {return ::powf  (x, y);}
INLINE double abs  (double x) {return ::fabs(x);}
INLINE double sign (double x) {return x<0?-1.0:1.0;}
INLINE double log2 (double x) {return ::log(x) / 0.69314718055994530941723212145818;}
INLINE double log10(double x) {return ::log10(x);}
INLINE double rcp  (double x) {return 1.0/x;}
INLINE double sqrt(double x) { return ::sqrt(x); }
INLINE double rsqrt(double x) { return 1.0 / ::sqrt(x); }
INLINE bool   select(bool s, bool  t , bool f) { return s ? t : f; }
INLINE int    select(bool s, int   t,   int f) { return s ? t : f; }
INLINE float  select(bool s, float t, float f) { return s ? t : f; }
INLINE double select(bool s, double t, double f) { return s ? t : f; }
TINLINE T fract(T x) {return x-floor(x);}
TINLINE T mod(T a, T b) {return a-b*floor(a/b);}
TINLINE T max(T a, T b) {return a<b? b:a;}
TINLINE T min(T a, T b) {return a<b? a:b;}
TINLINE T min(T a, T b, T c) {return min(min(a,b),c);}
TINLINE T max(T a, T b, T c) {return max(max(a,b),c);}
TINLINE T max(T a, T b, T c, T d) {return max(max(a,b),max(c,d));}
TINLINE T min(T a, T b, T c, T d) {return min(min(a,b),min(c,d));}
TINLINE T min(T a, T b, T c, T d, T e) {return min(min(min(a,b),min(c,d)),e);}
TINLINE T max(T a, T b, T c, T d, T e) {return max(max(max(a,b),max(c,d)),e);}
TINLINE T clamp(T x, T lower = T(zero), T upper = T(one)) {return max(lower,min(x,upper));}
TINLINE T deg2rad(T x) {return x * T(1.74532925199432957692e-2);}
TINLINE T rad2deg(T x) {return x * T(5.72957795130823208768e1);}
TINLINE T sin2cos(T x) {return sqrt(max(T(zero),T(one)-x*x));}
TINLINE T cos2sin(T x) {return sin2cos(x);}

/*-------------------------------------------------------------------------
 - 3D graphics related linear algebra (vectors, matrices)
 -------------------------------------------------------------------------*/
template<typename T> struct vec2;
template<typename T> struct vec3;
template<typename T> struct vec4;

// macros shared for swizzle declarations and definitions
#define sw21(A) sw22(A,x) sw22(A,y)
#define sw20 sw21(x) sw21(y)
#define sw31(A) sw32(A,x) sw32(A,y) sw32(A,z)
#define sw30 sw31(x) sw31(y) sw31(z)
#define sw41(A) sw42(A,x) sw42(A,y) sw42(A,z) sw42(A,w)
#define sw40 sw41(x) sw41(y) sw41(z) sw41(w)

#if defined(__WIN32__)
  #undef min
  #undef max
#if defined(__MSVC__)
  INLINE bool finite (float x) {return _finite(x) != 0;}
#endif
#endif

// make the code terser
#define v2 vec2<T>
#define v3 vec3<T>
#define v4 vec4<T>
#define m33 mat3x3<T>
#define m44 mat4x4<T>
#define v2arg const vec2<T>&
#define v3arg const vec3<T>&
#define v4arg const vec4<T>&
#define m33arg const mat3x3<T>&
#define m44arg const mat4x4<T>&
#define q3arg const quat<T>&
#define q3 quat<T>

template<typename T> struct vec2 {
  typedef T scalar;
  enum { channelnum = 2 };
  T x, y;
  INLINE vec2(void) {}
  INLINE vec2(const vec2& v) {x = v.x; y = v.y;}
  UINLINE vec2 (const vec2<U>& a) : x(T(a.x)), y(T(a.y)) {}
  UINLINE vec2& op= (const vec2<U>& v) {x = v.x; y = v.y; return *this;}
  INLINE explicit vec2(const T &a) : x(a), y(a) {}
  INLINE explicit vec2(const T &x, const T &y) : x(x), y(y) {}
  INLINE explicit vec2(const T *a, int stride = 1) : x(a[0]), y(a[stride]) {}
  INLINE vec2(zerotype) : x(zero),y(zero) {}
  INLINE vec2(onetype) : x(one),y(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
#define sw22(A,B) INLINE v2 A##B(void) const; // declare all swizzles
  sw20
#undef sw22
#define sw22(A,B) sw23(A,B,x) sw23(A,B,y)
#define sw23(A,B,C) INLINE v3 A##B##C(void) const;
  sw20
#undef sw23
#define sw23(A,B,C) sw24(A,B,C,x) sw24(A,B,C,y)
#define sw24(A,B,C,D) INLINE v4 A##B##C##D(void) const;
  sw20
#undef sw23
#undef sw24
#undef sw23
#undef sw22
};

#define OP(S) \
TINLINE v2 op S (v2arg a, v2arg b) {return v2(a.x S b.x, a.y S b.y);}\
TINLINE v2 op S (const T &a, v2arg b)  {return v2(a S b.x, a S b.y);}\
TINLINE v2 op S (v2arg a, const T &b)  {return v2(a.x S b, a.y S b);}\
TINLINE v2& op S##= (v2& a, v2arg b) {a.x S##= b.x; a.y S##= b.y; return a;}\
TINLINE v2& op S##= (v2& a, const T &b) {a.x S##= b; a.y S##= b; return a;}
OP(+) OP(/) OP(*) OP(-) OP(<<) OP(>>) OP(&) OP(|) OP(^)
#undef OP

#define OP(S) TINLINE v2 S (v2arg a) {return v2(S(a.x),S(a.y));}
OP(cos) OP(sin) OP(tan) OP(acos) OP(asin) OP(atan) OP(sinh) OP(cosh) OP(tanh)
OP(abs) OP(rcp) OP(sqrt) OP(rsqrt) OP(floor) OP(ceil) OP(log2) OP(log10)
#undef OP

#define OP(NAME,S) TINLINE vec2<bool> NAME(v2arg a, v2arg b)\
  {return vec2<bool>(a.x S b.x,a.y S b.y);}
OP(eq,==) OP(ne,!=) OP(lt,<) OP(gt,>) OP(ge,>=) OP(le,<=)
#undef OP

TINLINE v2 op+ (v2arg a)  {return v2(+a.x, +a.y);}
TINLINE v2 op- (v2arg a)  {return v2(-a.x, -a.y);}
TINLINE v2 min(v2arg a, v2arg b)  {return v2(min(a.x,b.x), min(a.y,b.y));}
TINLINE v2 max(v2arg a, v2arg b)  {return v2(max(a.x,b.x), max(a.y,b.y));}
TINLINE v2 clamp(v2arg v, v2arg m, v2arg M)  {return v2(clamp(v.x,m.x,M.x),clamp(v.y,m.y,M.y));}
TINLINE T dot(v2arg a, v2arg b)   {return a.x*b.x + a.y*b.y;}
TINLINE T length2(v2arg a)    {return dot(a,a);}
TINLINE T length(v2arg a)     {return sqrt(dot(a,a));}
TINLINE v2 normalize(v2arg a) {return a*rsqrt(dot(a,a));}
TINLINE T distance(v2arg a, v2arg b) {return length(a-b);}
TINLINE T reduceadd(v2arg a) {return a.x+a.y;}
TINLINE T reducemul(v2arg a) {return a.x*a.y;}
TINLINE T reducemin(v2arg a) {return min(a.x, a.y);}
TINLINE T reducemax(v2arg a) {return max(a.x, a.y);}
TINLINE v2 select (bool s, v2arg t, v2arg f) { return v2(select(s,t.x,f.x), select(s,t.y,f.y)); }
TINLINE v2 select (vec2<bool> s, v2arg t, v2arg f) { return v2(select(s.x,t.x,f.x), select(s.y,t.y,f.y)); }
INLINE bool any(const vec2<bool> &v) {return v.x||v.y;}
INLINE bool all(const vec2<bool> &v) {return v.x&&v.y;}
TINLINE bool op== (v2arg a, v2arg b) {return all(eq(a,b)); }
TINLINE bool op!= (v2arg a, v2arg b) {return any(ne(a,b)); }

template<typename T> struct vec3 {
  typedef T scalar;
  enum { channelnum = 3 };
  T x, y, z;
  INLINE vec3(void) {}
  INLINE vec3(const vec3& v) {x = v.x; y = v.y; z = v.z;}
  UINLINE vec3(const vec3<U>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)) {}
  UINLINE vec3& op= (const vec3<U>& v) {x = v.x; y = v.y; z = v.z; return *this;}
  INLINE explicit vec3(const T &a) : x(a), y(a), z(a) {}
  INLINE explicit vec3(const T &x, const T &y, const T &z) : x(x), y(y), z(z) {}
  INLINE explicit vec3(const T* a, int stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]) {}
  INLINE vec3 (zerotype) : x(zero),y(zero),z(zero) {}
  INLINE vec3 (onetype)  : x(one),y(one),z(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
#define sw32(A,B) INLINE v2 A##B(void) const; // declare all swizzles
  sw30
#undef sw32
#define sw32(A,B) sw33(A,B,x) sw33(A,B,y) sw33(A,B,z)
#define sw33(A,B,C) INLINE v3 A##B##C(void) const;
  sw30
#undef sw33
#define sw33(A,B,C) sw34(A,B,C,x) sw34(A,B,C,y) sw34(A,B,C,z)
#define sw34(A,B,C,D) INLINE v4 A##B##C##D(void) const;
  sw30
#undef sw33
#undef sw34
#undef sw33
#undef sw32
};

#define OP(S) \
TINLINE v3 op S (v3arg a, v3arg b) {return v3(a.x S b.x, a.y S b.y, a.z S b.z);}\
TINLINE v3 op S (const T &a, v3arg b)  {return v3(a S b.x, a S b.y, a S b.z);}\
TINLINE v3 op S (v3arg a, const T &b)  {return v3(a.x S b, a.y S b, a.z S b);}\
TINLINE v3& op S##= (v3& a, v3arg b) {a.x S##= b.x; a.y S##= b.y; a.z S##= b.z; return a;}\
TINLINE v3& op S##= (v3& a, const T &b) {a.x S##= b; a.y S##= b; a.z S##= b; return a;}
OP(+) OP(/) OP(*) OP(-) OP(<<) OP(>>) OP(&) OP(|) OP(^) OP(%)
#undef OP

#define OP(S) TINLINE v3 S (v3arg a) {return v3(S(a.x),S(a.y),S(a.z));}
OP(cos) OP(sin) OP(tan) OP(acos) OP(asin) OP(atan) OP(sinh) OP(cosh) OP(tanh)
OP(abs) OP(rcp) OP(sqrt) OP(rsqrt) OP(floor) OP(ceil) OP(log2) OP(log10)
#undef OP

#define OP(NAME,S)\
TINLINE vec3<bool> NAME(v3arg a, v3arg b) {return vec3<bool>(a.x S b.x,a.y S b.y,a.z S b.z);}\
TINLINE vec3<bool> NAME(const T &a, v3arg b) {return vec3<bool>(a S b.x,a S b.y,a S b.z);}\
TINLINE vec3<bool> NAME(v3arg a, const T &b) {return vec3<bool>(a.x S b,a.y S b,a.z S b);}
OP(eq,==) OP(ne,!=) OP(lt,<) OP(gt,>) OP(ge,>=) OP(le,<=)
#undef OP

TINLINE v3 op+ (v3arg a) {return v3(+a.x, +a.y, +a.z);}
TINLINE v3 op- (v3arg a) {return v3(-a.x, -a.y, -a.z);}
TINLINE T reduceadd(v3arg a) {return a.x+a.y+a.z;}
TINLINE T reducemul(v3arg a) {return a.x*a.y*a.z;}
TINLINE T reducemin(v3arg a) {return min(a.x,a.y,a.z);}
TINLINE T reducemax(v3arg a) {return max(a.x,a.y,a.z);}
TINLINE v3 min(v3arg a, v3arg b) {return v3(min(a.x,b.x),min(a.y,b.y),min(a.z,b.z));}
TINLINE v3 max(v3arg a, v3arg b) {return v3(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));}
TINLINE v3 clamp(v3arg v, v3arg m, v3arg M)  {return v3(clamp(v.x,m.x,M.x),clamp(v.y,m.y,M.y),clamp(v.z,m.z,M.z));}
TINLINE v3 select(bool s, v3arg t, v3arg f) { return v3(select(s,t.x,f.x), select(s,t.y,f.y), select(s,t.z,f.z)); }
TINLINE v3 select(vec3<bool> s, v3arg t, v3arg f) { return v3(select(s.x,t.x,f.x), select(s.y,t.y,f.y), select(s.z,t.z,f.z)); }
TINLINE T length2(v3arg a) {return dot(a,a);}
TINLINE T length(v3arg a) {return sqrt(dot(a,a));}
TINLINE T dot(v3arg a, v3arg b) {return a.x*b.x + a.y*b.y + a.z*b.z;}
TINLINE T distance(v3arg a, v3arg b) {return length(a-b);}
TINLINE T distance2(v3arg a, v3arg b) {return dot(a-b,a-b);}
TINLINE v3 normalize(v3arg a) {return a*rsqrt(dot(a,a));}
TINLINE v3 cross(v3arg a, v3arg b) { return v3(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
INLINE bool any(const vec3<bool> &v) {return v.x||v.y||v.z;}
INLINE bool all(const vec3<bool> &v) {return v.x&&v.y&&v.z;}
TINLINE bool op== (v3arg a, v3arg b) {return all(eq(a,b)); }
TINLINE bool op!= (v3arg a, v3arg b) {return any(ne(a,b)); }

template<typename T> struct vec4 {
  typedef T scalar;
  enum { channelnum = 4 };
  T x, y, z, w;
  INLINE vec4(void) {}
  INLINE vec4(const vec4 &v) {x = v.x; y = v.y; z = v.z; w = v.w;}
  INLINE vec4(const vec3<T> &v, float f) {x = v.x; y = v.y; z = v.z; w = f;}
  UINLINE vec4(const vec4<U>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)), w(T(a.w)) {}
  UINLINE vec4& op= (const vec4<U>& v) {x = v.x; y = v.y; z = v.z; w = v.w; return *this;}
  INLINE explicit vec4(const T &a) : x(a), y(a), z(a), w(a) {}
  INLINE explicit vec4(const T &x, const T &y, const T &z, const T &w) : x(x), y(y), z(z), w(w) {}
  INLINE explicit vec4(const T *a, int stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]), w(a[3*stride]) {}
  INLINE vec4 (zerotype) : x(zero),y(zero),z(zero),w(zero) {}
  INLINE vec4 (onetype)  : x(one),y(one),z(one),w(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
#define sw42(A,B) INLINE v2 A##B(void) const; // declare all swizzles
  sw40
#undef sw42
#define sw42(A,B) sw43(A,B,x) sw43(A,B,y) sw43(A,B,z) sw43(A,B,w)
#define sw43(A,B,C) INLINE v3 A##B##C(void) const;
  sw40
#undef sw43
#define sw43(A,B,C) sw44(A,B,C,x) sw44(A,B,C,y) sw44(A,B,C,z) sw44(A,B,C,w)
#define sw44(A,B,C,D) INLINE v4 A##B##C##D(void) const;
  sw40
#undef sw43
#undef sw44
#undef sw43
#undef sw42
};

#define OP(S) \
TINLINE v4 op S (v4arg a, v4arg b) {return v4(a.x S b.x, a.y S b.y, a.z S b.z, a.w S b.w);}\
TINLINE v4 op S (const T &a, v4arg b)  {return v4(a S b.x, a S b.y, a S b.z, a S b.w);}\
TINLINE v4 op S (v4arg a, const T &b)  {return v4(a.x S b, a.y S b, a.z S b, a.w S b);}\
TINLINE v4& op S##= (v4& a, v4arg b) {a.x S##= b.x; a.y S##= b.y; a.z S##= b.z; a.w S##= b.w; return a;}\
TINLINE v4& op S##= (v4& a, const T &b) {a.x S##= b; a.y S##= b; a.z S##= b; a.w S##= b; return a;}
OP(+) OP(/) OP(*) OP(-) OP(<<) OP(>>) OP(&) OP(|) OP(^)
#undef OP

#define OP(S) TINLINE v4 S (v4arg a) {return v4(S(a.x),S(a.y),S(a.z),S(a.w));}
OP(cos) OP(sin) OP(tan) OP(acos) OP(asin) OP(atan) OP(sinh) OP(cosh) OP(tanh)
OP(abs) OP(rcp) OP(sqrt) OP(rsqrt) OP(floor) OP(ceil) OP(log2) OP(log10)
#undef OP

#define OP(NAME,S) TINLINE vec4<bool> NAME(v4arg a, v4arg b)\
  {return vec4<bool>(a.x S b.x,a.y S b.y,a.z S b.z,a.w S b.w);}
OP(eq,==) OP(ne,!=) OP(lt,<) OP(gt,>) OP(ge,>=) OP(le,<=)
#undef OP

TINLINE v4 op+ (v4arg a) {return v4(+a.x, +a.y, +a.z, +a.w);}
TINLINE v4 op- (v4arg a) {return v4(-a.x, -a.y, -a.z, -a.w);}
TINLINE T reduceadd(v4arg a) {return a.x + a.y + a.z + a.w;}
TINLINE T reducemul(v4arg a) {return a.x * a.y * a.z * a.w;}
TINLINE T reducemin(v4arg a) {return min(a.x, a.y, a.z, a.w);}
TINLINE T reducemax(v4arg a) {return max(a.x, a.y, a.z, a.w);}
TINLINE v4 min(v4arg a, v4arg b) {return v4(min(a.x,b.x), min(a.y,b.y), min(a.z,b.z), min(a.w,b.w));}
TINLINE v4 max(v4arg a, v4arg b) {return v4(max(a.x,b.x), max(a.y,b.y), max(a.z,b.z), max(a.w,b.w));}
TINLINE v4 clamp(v4arg v, v4arg m, v4arg M)  {return v4(clamp(v.x,m.x,M.x),clamp(v.y,m.y,M.y),clamp(v.z,m.z,M.z),clamp(v.w,m.w,M.w));}
TINLINE T length2(v4arg a) {return dot(a,a);}
TINLINE T length(v4arg a) {return sqrt(dot(a,a));}
TINLINE T dot (v4arg a, v4arg b) {return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;}
TINLINE v4 normalize(v4arg a) {return a*rsqrt(dot(a,a));}
TINLINE T distance (v4arg a, v4arg b) {return length(a-b);}
TINLINE v4 select (bool s, v4arg t, v4arg f) { return v4(select(s,t.x,f.x), select(s,t.y,f.y), select(s,t.z,f.z), select(s,t.w,f.w)); }
TINLINE v4 select (vec4<bool> s, v4arg t, v4arg f) { return v4(select(s.x,t.x,f.x), select(s.y,t.y,f.y), select(s.z,t.z,f.z), select(s.w,t.w,f.w)); }
INLINE bool any(const vec4<bool> &v) {return v.x||v.y||v.z||v.w;}
INLINE bool all(const vec4<bool> &v) {return v.x&&v.y&&v.z&&v.w;}
TINLINE bool op== (v4arg a, v4arg b) {return all(eq(a,b)); }
TINLINE bool op!= (v4arg a, v4arg b) {return any(ne(a,b)); }

template<typename T> struct mat3x3 {
  vec3<T> vx,vy,vz;
  mat3x3(v3arg n);
  INLINE mat3x3(void) {}
  INLINE mat3x3(const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz;}
  INLINE mat3x3& op= (const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz; return *this;}
  UINLINE explicit mat3x3(const mat3x3<U> &s) : vx(s.vx), vy(s.vy), vz(s.vz) {}
  INLINE mat3x3(v3arg vx, v3arg vy, v3arg vz) : vx(vx), vy(vy), vz(vz) {}
  INLINE mat3x3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22) :
    vx(m00,m10,m20), vy(m01,m11,m21), vz(m02,m12,m22) {}
  INLINE mat3x3(zerotype) : vx(zero),vy(zero),vz(zero) {}
  INLINE mat3x3(onetype)  : vx(one,zero,zero),vy(zero,one,zero),vz(zero,zero,one) {}
  INLINE mat3x3 adjoint(void) const {
    return mat3x3(cross(vy,vz), cross(vz,vx), cross(vx,vy)).transposed();
  }
  INLINE mat3x3 transposed(void) const {
    return mat3x3(vx.x,vx.y,vx.z, vy.x,vy.y,vy.z, vz.x,vz.y,vz.z);
  }
  INLINE mat3x3 inverse(void) const {return rcp(det())*adjoint();}
  INLINE T det(void) const {return dot(vx,cross(vy,vz));}
  static INLINE mat3x3 scale(v3arg s) {
    return mat3x3(s.x,zero,zero,zero,s.y,zero,zero,zero,s.z);
  }
  static INLINE mat3x3 rotate(T r, v3arg _u) {
    const v3 u = normalize(_u);
    const T s = sin(deg2rad(r)), c = cos(deg2rad(r));
    return mat3x3(u.x*u.x+(T(one)-u.x*u.x)*c,u.x*u.y*(T(one)-c)-u.z*s,  u.x*u.z*(T(one)-c)+u.y*s,
                  u.x*u.y*(T(one)-c)+u.z*s,  u.y*u.y+(T(one)-u.y*u.y)*c,u.y*u.z*(T(one)-c)-u.x*s,
                  u.x*u.z*(T(one)-c)-u.y*s,  u.y*u.z*(T(one)-c)+u.x*s,  u.z*u.z+(T(one)-u.z*u.z)*c);
  }
};

TINLINE m33 op- (m33arg a) {return m33(-a.vx,-a.vy,-a.vz);}
TINLINE m33 op+ (m33arg a) {return m33(+a.vx,+a.vy,+a.vz);}
TINLINE m33 rcp (m33arg a) {return a.inverse();}
TINLINE m33 op+ (m33arg a, m33arg b) {return m33(a.vx+b.vx,a.vy+b.vy,a.vz+b.vz);}
TINLINE m33 op- (m33arg a, m33arg b) {return m33(a.vx-b.vx,a.vy-b.vy,a.vz-b.vz);}
TINLINE m33 op* (m33arg a, m33arg b) {return m33(a*b.vx, a*b.vy, a*b.vz);}
TINLINE m33 op/ (m33arg a, m33arg b) {return a*rcp(b);}
TINLINE v3  op* (m33arg a, v3arg  b) {return b.x*a.vx + b.y*a.vy + b.z*a.vz;}
TINLINE m33 op* (T a, m33arg b)   {return m33(a*b.vx, a*b.vy, a*b.vz);}
TINLINE m33 op/ (m33arg a, T b)   {return a * rcp(b);}
TINLINE m33& op/= (m33& a, T b)   {return a = a/b;}
TINLINE m33& op*= (m33& a, T b)   {return a = a*b;}
TINLINE m33& op*= (m33& a, m33 b) {return a = a*b;}
TINLINE m33& op/= (m33& a, m33 b) {return a = a/b;}
TINLINE bool op== (m33arg a, m33arg b)  {return (a.vx==b.vx) && (a.vy == b.vy) && (a.vz == b.vz);}
TINLINE bool op!= (m33arg a, m33arg b)  {return (a.vx!=b.vx) || (a.vy != b.vy) || (a.vz != b.vz);}
TINLINE v3 xfmpoint (m33arg s, v3arg a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
TINLINE v3 xfmvector(m33arg s, v3arg a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
TINLINE v3 xfmnormal(m33arg s, v3arg a) {return xfmvector(s.inverse().transposed(),a);}
TINLINE m33 frame(v3arg N) {
  const v3 dx0 = cross(v3(T(one),T(zero),T(zero)),N);
  const v3 dx1 = cross(v3(T(zero),T(one),T(zero)),N);
  const v3 dx = normalize(select(dot(dx0,dx0) > dot(dx1,dx1),dx0,dx1));
  const v3 dy = normalize(cross(N,dx));
  return m33(dx,dy,N);
}


template<typename T> struct mat4x4 {
  vec4<T> vx,vy,vz,vw;
  INLINE mat4x4(void) {}
  INLINE mat4x4(m44arg m) {vx=m.vx; vy=m.vy; vz=m.vz; vw=m.vw;}
  UINLINE mat4x4(const mat4x4<U> &m) :
    vx(v4(m.vx)), vy(v4(m.vy)), vz(v4(m.vz)), vw(v4(m.vw)) {}
  UINLINE explicit mat4x4 (U s) {
    vx = v4(T(s), zero, zero, zero);
    vy = v4(zero, T(s), zero, zero);
    vz = v4(zero, zero, T(s), zero);
    vw = v4(zero, zero, zero, T(s));
  }
  INLINE mat4x4(T s) {
    vx = v4(s, zero, zero, zero);
    vy = v4(zero, s, zero, zero);
    vz = v4(zero, zero, s, zero);
    vw = v4(zero, zero, zero, s);
  }
  INLINE mat4x4(v4arg c0, v4arg c1, v4arg c2, v4arg c3) :
    vx(c0), vy(c1), vz(c2), vw(c3) {}
  INLINE mat4x4 (m33arg m) {
    vx = v4(m.vx, zero);
    vy = v4(m.vy, zero);
    vz = v4(m.vz, zero);
    vw = v4(zero, zero, zero, one);
  }
  INLINE mat4x4(zerotype) : vx(zero),vy(zero),vz(zero),vw(zero) {}
  INLINE mat4x4(onetype) {
    vx = v4(one, zero, zero, zero);
    vy = v4(zero, one, zero, zero);
    vz = v4(zero, zero, one, zero);
    vw = v4(zero, zero, zero, one);
  }
  m44& op= (const m44 &m) {vx=m.vx;vy=m.vy;vz=m.vz;vw=m.vw;return *this;}
  m44 inverse(void) const;
  static INLINE m44 translate(v3arg v) {
    m44 m(one);
    m.vw = v4(v.x,v.y,v.z,one);
    return m;
  }
  static INLINE m44 rotate(T angle, v3arg v) {
    m44 rot(zero), dst(zero);
    const T a = deg2rad(angle);
    const T c = cos(a);
    const T s = sin(a);
    const v3 axis = normalize(v);
    const v3 temp = (T(one) - c) * axis;
    rot.vx.x = c + temp.x*axis.x;
    rot.vx.y = temp.x*axis.y + s*axis.z;
    rot.vx.z = temp.x*axis.z - s*axis.y;
    rot.vy.x = temp.y*axis.x - s*axis.z;
    rot.vy.y = c + temp.y*axis.y;
    rot.vy.z = temp.y*axis.z + s*axis.x;
    rot.vz.x = temp.z*axis.x + s*axis.y;
    rot.vz.y = temp.z*axis.y - s*axis.x;
    rot.vz.z = c + temp.z*axis.z;
    rot.vw.w = one;
    return rot;
  }
  INLINE v4& op[] (int i) {return (&vx)[i];}
  INLINE const v4& op[] (int i) const {return (&vx)[i];}
};

TINLINE m44 op- (m44arg a) {return m44(-a.vx,-a.vy,-a.vz,-a.vw);}
TINLINE m44 op+ (m44arg a) {return m44(+a.vx,+a.vy,+a.vz,+a.vw);}
TINLINE m44 op+ (m44arg m, T s) {return m44(m.vx+s, m.vy+s, m.vz+s, m.vw+s);}
TINLINE m44 op- (m44arg m, T s) {return m44(m.vx-s, m.vy-s, m.vz-s, m.vw-s);}
TINLINE m44 op* (m44arg m, T s) {return m44(m.vx*s, m.vy*s, m.vz*s, m.vw*s);}
TINLINE m44 op/ (m44arg m, T s) {return m44(m.vx/s, m.vy/s, m.vz/s, m.vw/s);}
TINLINE m44 op+ (T s, m44arg m) {return m44(m.vx+s, m.vy+s, m.vz+s, m.vw+s);}
TINLINE m44 op- (T s, m44arg m) {return m44(m.vx-s, m.vy-s, m.vz-s, m.vw-s);}
TINLINE m44 op* (T s, m44arg m) {return m44(m.vx*s, m.vy*s, m.vz*s, m.vw*s);}
TINLINE m44 op+ (m44arg m, m44arg n) {return m44(m.vx+n.vx, m.vy+n.vy, m.vz+n.vz, m.vw+n.vw);}
TINLINE m44 op- (m44arg m, m44arg n) {return m44(m.vx-n.vx, m.vy-n.vy, m.vz-n.vz, m.vw-n.vw);}
TINLINE v4  op* (m44arg m, v4arg v) {
  return v4(m.vx.x*v.x + m.vy.x*v.y + m.vz.x*v.z + m.vw.x*v.w,
            m.vx.y*v.x + m.vy.y*v.y + m.vz.y*v.z + m.vw.y*v.w,
            m.vx.z*v.x + m.vy.z*v.y + m.vz.z*v.z + m.vw.z*v.w,
            m.vx.w*v.x + m.vy.w*v.y + m.vz.w*v.z + m.vw.w*v.w);
}
TINLINE v4 op* (v4arg v, m44arg m) {
  return v4(dot(m.vx,v), dot(m.vy,v), dot(m.vz,v), dot(m.vw,v));
}
TNOINLINE m44 op* (m44arg m, m44arg n) {
  const v4 a0 = m.vx, a1 = m.vy, a2 = m.vz, a3 = m.vw;
  const v4 b0 = n.vx, b1 = n.vy, b2 = n.vz, b3 = n.vw;
  return m44(a0*b0.x + a1*b0.y + a2*b0.z + a3*b0.w,
             a0*b1.x + a1*b1.y + a2*b1.z + a3*b1.w,
             a0*b2.x + a1*b2.y + a2*b2.z + a3*b2.w,
             a0*b3.x + a1*b3.y + a2*b3.z + a3*b3.w);
}
TINLINE m44& op*= (m44& a, T b) {return a = a*b;}
TINLINE m44& op/= (m44& a, T b) {return a = a/b;}
TINLINE m44& op*= (m44& a, m44arg b) {return a = a*b;}
TINLINE m44& op/= (m44& a, m44arg b) {return a = a/b;}
TINLINE v4 op/ (m44arg m, v4arg v) {return m.inverse() * v;}
TINLINE v4 op/ (v4arg v, m44arg m) {return v * m.inverse();}
TINLINE m44 op/ (m44arg m, m44arg n) {return m * n.inverse();}
TINLINE bool eq (m44arg m, m44arg n) {return (m.vx==n.x) && (m.vy==n.y) && (m.vz==n.z) && (m.vw==n.w);}
TINLINE bool ne (m44arg m, m44arg n) {return (m.vx!=n.x) || (m.vy!=n.y) || (m.vz!=n.z) || (m.vw!=n.w);}
TINLINE m44 lookat(v3arg eye, v3arg center, v3arg up) {
  const v3 f = normalize(center - eye);
  const v3 u = normalize(up);
  const v3 s = normalize(cross(f, u));
  const v3 t = cross(s, f);
  m44 dst(one);
  dst.vx.x = s.x; dst.vy.x = s.y; dst.vz.x = s.z;
  dst.vx.y = t.x; dst.vy.y = t.y; dst.vz.y = t.z;
  dst.vx.z =-f.x; dst.vy.z =-f.y; dst.vz.z =-f.z;
  return translate(dst, -eye);
}
TINLINE m44 perspective(T fovy, T aspect, T znear, T zfar) {
  const T range = tan(deg2rad(fovy / T(two))) * znear;
  const T left = -range * aspect;
  const T right = range * aspect;
  const T bottom = -range;
  const T top = range;
  m44 dst(zero);
  dst.vx.x = (T(two) * znear) / (right - left);
  dst.vy.y = (T(two) * znear) / (top - bottom);
  dst.vz.z = -(zfar + znear) / (zfar - znear);
  dst.vz.w = -T(one);
  dst.vw.z = -(T(two) * zfar * znear) / (zfar - znear);
  return dst;
}
TINLINE m44 ortho(T left, T right, T bottom, T top, T znear, T zfar) {
  m44 dst(one);
  dst.vx.x = T(two) / (right - left);
  dst.vy.y = T(two) / (top - bottom);
  dst.vz.z = -T(two) / (zfar - znear);
  dst.vw.x = -(right + left) / (right - left);
  dst.vw.y = -(top + bottom) / (top - bottom);
  dst.vw.z = -(zfar + znear) / (zfar - znear);
  return dst;
}
TINLINE m44 scale(m44arg m, v3arg v) {
  m44 dst;
  dst.vx = m.vx*v.x;
  dst.vy = m.vy*v.y;
  dst.vz = m.vz*v.z;
  dst.vw = m.vw;
  return dst;
}
TINLINE v3 unproject(v3arg win, m44arg model, m44arg proj, const vec4<int> &viewport) {
  const m44 p = proj*model;
  const m44 final = p.inverse();
  v4 in(win.x, win.y, win.z, T(one));
  in.x = T(two) * (win.x - T(viewport.x)) / T(viewport.z) - T(one);
  in.y = T(two) * (win.y - T(viewport.y)) / T(viewport.w) - T(one);
  in.z = T(two) * win.z - T(one);
  const v4 out = final*in;
  return out.xyz() / out.w;
}


template<typename T> struct quat {
  typedef vec3<T> Vector;
  T r, i, j, k;

  INLINE quat(void) {}
  INLINE quat(const quat &other) {r=other.r; i=other.i; j=other.j; k=other.k;}
  INLINE quat &op= (const quat &other) {r=other.r; i=other.i; j=other.j; k=other.k; return *this;}
  INLINE quat(const T &r) : r(r), i(zero), j(zero), k(zero) {}
  INLINE explicit quat(v3arg v) : r(zero), i(v.x), j(v.y), k(v.z) {}
  INLINE quat(const T &r, const T &i, const T &j , const T &k) : r(r), i(i), j(j), k(k) {}
  INLINE quat(const T &r, v3arg v) : r(r), i(v.x), j(v.y), k(v.z) {}
  INLINE quat(v3arg vx, v3arg vy, v3arg vz);
  INLINE quat(const T &yaw, const T &pitch, const T &roll);
  INLINE quat(zerotype) : r(zero), i(zero), j(zero), k(zero) {}
  INLINE quat(onetype) : r(one), i(zero), j(zero), k(zero) {}
  static INLINE quat rotate(v3arg u, const T &r) {
    return q3(cos(T(0.5)*r),sin(T(0.5)*r)*normalize(u));
  }
  INLINE const vec3<T> v() const { return vec3<T>(i, j, k); }
};

TINLINE q3 op* (const T &a, q3arg b) {return q3(a*b.r, a*b.i, a*b.j, a*b.k);}
TINLINE q3 op* (q3arg a, const T &b) {return q3(a.r*b, a.i*b, a.j*b, a.k*b);}

TINLINE q3 op+ (q3arg a) { return q3(+a.r, +a.i, +a.j, +a.k); }
TINLINE q3 op- (q3arg a) { return q3(-a.r, -a.i, -a.j, -a.k); }
TINLINE q3 conj(q3arg a) { return q3(a.r, -a.i, -a.j, -a.k); }
TINLINE T abs(q3arg a) { return sqrt(a.r*a.r + a.i*a.i + a.j*a.j + a.k*a.k); }
TINLINE q3 rcp(q3arg a) { return conj(a)*rcp(a.r*a.r + a.i*a.i + a.j*a.j + a.k*a.k); }
TINLINE q3 normalize (q3arg a) { return a*rsqrt(a.r*a.r + a.i*a.i + a.j*a.j + a.k*a.k); }

#define OP(S)\
TINLINE q3 op S(const T &a, q3arg b) {return q3(a S b.r, S b.i, S b.j, S b.k);}\
TINLINE q3 op S(q3arg a, const T &b) {return q3(a.r S b, a.i, a.j, a.k);}\
TINLINE q3 op S(q3arg a, q3arg b)  {return q3(a.r S b.r, a.i S b.i, a.j S b.j, a.k S b.k);}
OP(+) OP(-)
#undef OP

#define OP(S)\
TINLINE q3 &op S##= (q3 &a, const T &b) { return a = a S b; }\
TINLINE q3 &op S##= (q3 &a, q3arg b) { return a = a S b; }
OP(+) OP(-) OP(*) OP(/)
#undef OP

TINLINE q3 op/ (const T &a, q3arg b) { return a*rcp(b); }
TINLINE q3 op/ (q3arg a, const T &b) { return a*rcp(b); }
TINLINE q3 op/ (q3arg a, q3arg b) { return a*rcp(b); }
TINLINE vec3<T> op* (q3arg a, v3arg b) { return (a*q3(b)*conj(a)).v(); }
TINLINE q3 op* (q3arg a, q3arg b) {
  return q3(a.r*b.r - a.i*b.i - a.j*b.j - a.k*b.k,
            a.r*b.i + a.i*b.r + a.j*b.k - a.k*b.j,
            a.r*b.j - a.i*b.k + a.j*b.r + a.k*b.i,
            a.r*b.k + a.i*b.j - a.j*b.i + a.k*b.r);
}

TINLINE vec3<T> xfmpoint (q3arg a, v3arg b) {return (a*q3(b)*conj(a)).v();}
TINLINE vec3<T> xfmvector(q3arg a, v3arg b) {return (a*q3(b)*conj(a)).v();}
TINLINE vec3<T> xfmnormal(q3arg a, v3arg b) {return (a*q3(b)*conj(a)).v();}

TINLINE bool op== (q3arg a, q3arg b) {return a.r==b.r && a.i==b.i && a.j==b.j && a.k==b.k;}
TINLINE bool op!= (q3arg a, q3arg b) {return a.r!=b.r || a.i!=b.i || a.j!=b.j || a.k!=b.k;}

template<typename T> q3::quat(v3arg vx, v3arg vy, v3arg vz) {
  if (vx.x + vy.y + vz.z >= T(zero)) {
    const T t = T(one) + (vx.x + vy.y + vz.z);
    const T s = rsqrt(t)*T(0.5f);
    r = t*s;
    i = (vy.z - vz.y)*s;
    j = (vz.x - vx.z)*s;
    k = (vx.y - vy.x)*s;
  } else if (vx.x >= max(vy.y, vz.z)) {
    const T t = (T(one) + vx.x) - (vy.y + vz.z);
    const T s = rsqrt(t)*T(0.5f);
    r = (vy.z - vz.y)*s;
    i = t*s;
    j = (vx.y + vy.x)*s;
    k = (vz.x + vx.z)*s;
  } else if (vy.y >= vz.z) { // if (vy.y >= max(vz.z, vx.x))
    const T t = (T(one) + vy.y) - (vz.z + vx.x);
    const T s = rsqrt(t)*T(0.5f);
    r = (vz.x - vx.z)*s;
    i = (vx.y + vy.x)*s;
    j = t*s;
    k = (vy.z + vz.y)*s;
  } else { //if (vz.z >= max(vy.y, vx.x))
    const T t = (T(one) + vz.z) - (vx.x + vy.y);
    const T s = rsqrt(t)*T(0.5f);
    r = (vx.y - vy.x)*s;
    i = (vz.x + vx.z)*s;
    j = (vy.z + vz.y)*s;
    k = t*s;
  }
}

template<typename T> q3::quat(const T &yaw, const T &pitch, const T &roll) {
  const T cya = cos(yaw  *T(0.5f));
  const T cpi = cos(pitch*T(0.5f));
  const T cro = cos(roll *T(0.5f));
  const T sya = sin(yaw  *T(0.5f));
  const T spi = sin(pitch*T(0.5f));
  const T sro = sin(roll *T(0.5f));
  r = cro*cya*cpi + sro*sya*spi;
  i = cro*cya*spi + sro*sya*cpi;
  j = cro*sya*cpi - sro*cya*spi;
  k = sro*cya*cpi - cro*sya*spi;
}

// define all swizzles for vec2
#define sw22(A,B) TINLINE v2 v2::A##B(void) const {return v2(A,B);}
sw20
#undef sw22
#define sw22(A,B) sw23(A,B,x) sw23(A,B,y)
#define sw23(A,B,C) TINLINE v3 v2::A##B##C(void) const {return v3(A,B,C);}
sw20
#undef sw23
#define sw23(A,B,C) sw24(A,B,C,x) sw24(A,B,C,y)
#define sw24(A,B,C,D) TINLINE v4 v2::A##B##C##D(void) const {return v4(A,B,C,D);}
sw20
#undef sw24
#undef sw23
#undef sw22

// define all swizzles for vec3
#define sw32(A,B) TINLINE v2 v3::A##B(void) const {return v2(A,B);}
sw30
#undef sw32
#define sw32(A,B) sw33(A,B,x) sw33(A,B,y) sw33(A,B,z)
#define sw33(A,B,C) TINLINE v3 v3::A##B##C(void) const {return v3(A,B,C);}
sw30
#undef sw33
#define sw33(A,B,C) sw34(A,B,C,x) sw34(A,B,C,y) sw34(A,B,C,z)
#define sw34(A,B,C,D) TINLINE v4 v3::A##B##C##D(void) const {return v4(A,B,C,D);}
sw30
#undef sw34
#undef sw33
#undef sw32

// define all swizzles for vec4
#define sw42(A,B) TINLINE v2 v4::A##B(void) const {return v2(A,B);}
sw40
#undef sw42
#define sw42(A,B) sw43(A,B,x) sw43(A,B,y) sw43(A,B,z) sw43(A,B,w)
#define sw43(A,B,C) TINLINE v3 v4::A##B##C(void) const {return v3(A,B,C);}
sw40
#undef sw43
#define sw43(A,B,C) sw44(A,B,C,x) sw44(A,B,C,y) sw44(A,B,C,z) sw44(A,B,C,w)
#define sw44(A,B,C,D) TINLINE v4 v4::A##B##C##D(void) const {return v4(A,B,C,D);}
sw40
#undef sw44
#undef sw43
#undef sw42

/*-------------------------------------------------------------------------
 - bare minimum interval arithmetic
 -------------------------------------------------------------------------*/
template <typename T> struct interval {
  INLINE interval(void) {}
  INLINE interval(T m, T M) : m(m), M(M) {}
  INLINE interval(zerotype) : m(zero), M(zero) {}
  INLINE interval(onetype) : m(one), M(one) {}
  T m, M;
};

#define itvarg const interval<T>&
#define itv interval<T>
TINLINE itv op- (itvarg x)  {return itv(-x.M,-x.m);}
TINLINE itv op+ (itvarg x)  {return itv(+x.m,+x.M);}
TINLINE itv op+ (itvarg x, itvarg y) {return itv(x.m+y.m,x.M+y.M);}
TINLINE itv op- (itvarg x, itvarg y) {return itv(x.m+(-y.m),x.M+(-y.M));}
TINLINE itv op* (itvarg x, itvarg y) {
  return itv(min(x.m*y.m, x.M*y.m, x.M*y.M, x.m*y.M),
             max(x.m*y.m, x.M*y.m, x.M*y.M, x.m*y.M));
}
TINLINE itv rcp (itvarg x) {return itv(rcp(x.M),rcp(x.m));}
TINLINE itv U(itvarg x, itvarg y) {return itv(min(x.m,y.m),max(x.M,y.M)); }
TINLINE itv U(itvarg x, itvarg y, itvarg z) {return U(U(x,y),z);}
TINLINE itv U(itvarg x, itvarg y, itvarg z, itvarg w) {return U(U(x,y,z),w);}
TINLINE itv I(itvarg x, itvarg y) {return itv(max(x.m,y.m),min(x.M,y.M)); }
TINLINE itv I(itvarg x, itvarg y, itvarg z) {return I(I(x,y),z);}
TINLINE itv I(itvarg x, itvarg y, itvarg z, itvarg w) {return I(I(x,y),I(z,w));}
TINLINE bool empty(itvarg x) {return x.m > x.M;}
TINLINE vec4<itv> makeinterval(vec4<T> m, vec4<T> M) {
  return vec4<itv>(itv(m.x,M.x), itv(m.y,M.y), itv(m.z,M.z), itv(m.w,M.w));
}
TINLINE vec3<itv> makeinterval(vec3<T> m, vec3<T> M) {
  return vec3<itv>(itv(m.x,M.x), itv(m.y,M.y), itv(m.z,M.z));
}
TINLINE vec2<itv> makeinterval(vec2<T> m, vec2<T> M) {
  return vec2<itv>(itv(m.x,M.x), itv(m.y,M.y));
}
#undef itvarg
#undef itv

// commonly used types
typedef vec2<bool> vec2b;
typedef vec2<int> vec2i;
typedef vec2<float> vec2f;
typedef vec2<double> vec2d;
typedef vec3<bool> vec3b;
typedef vec3<int> vec3i;
typedef vec3<float> vec3f;
typedef vec3<double> vec3d;
typedef vec4<bool> vec4b;
typedef vec4<int> vec4i;
typedef vec4<float> vec4f;
typedef vec4<double> vec4d;
typedef mat3x3<float> mat3x3f;
typedef mat3x3<double> mat3x3d;
typedef mat4x4<float> mat4x4f;
typedef mat4x4<double> mat4x4d;
typedef quat<float>  quat3f;
typedef quat<double> quat3d;
typedef interval<float> intervalf;
typedef interval<int> intervali;
typedef vec3<intervalf> interval3f;
typedef vec3<intervali> interval3i;

#undef TINLINE
#undef TNOINLINE
#undef UINLINE
#undef op
#undef v2
#undef v3
#undef v4
#undef q3
#undef m33
#undef m44
#undef v2arg
#undef v3arg
#undef v4arg
#undef q3arg
#undef m33arg
#undef m44arg
#undef sw21
#undef sw20
#undef sw31
#undef sw30
#undef sw41
#undef sw40

/*-------------------------------------------------------------------------
 - simple fixed size array
 -------------------------------------------------------------------------*/
template <typename U, int n> struct array {
  typedef U scalar;
  template <typename... T> INLINE array(T... args) {set(0,args...);}
  INLINE array(zerotype) { loopi(n) v[i] = U(zero); }
  INLINE array(onetype) { loopi(n) v[i] = U(one); }
  template <typename First, typename... Rest>
  INLINE void set(int i, First first, Rest... rest) {
    assign(first, i);
    set(i,rest...);
  }
  INLINE void assign(U x, int &i) {this->v[i++] = x;}
  INLINE void assign(vec2<U> u, int &i) {v[i++]=u.x; v[i++]=u.y;}
  INLINE void assign(vec3<U> u, int &i) {v[i++]=u.x; v[i++]=u.y; v[i++]=u.z;}
  INLINE void assign(vec4<U> u, int &i) {v[i++]=u.x; v[i++]=u.y; v[i++]=u.z; v[i++]=u.w;}
  INLINE void set(int i) {}
  U &operator[] (int i) {assert(i<n&&i>=0);return v[i];}
  const U &operator[] (int i) const {assert(i<n&&i>=0);return v[i];}
  U v[n];
};
template <typename U, int n>
INLINE bool operator!= (const array<U,n> &v0, const array<U,n> &v1) {
  loopi(n) if (v0.v[i] != v1.v[i]) return true;
  return false;
}
template <typename U, int n>
INLINE bool operator== (const array<U,n> &v0, const array<U,n> &v1) {
  return !(v0!=v1);
}

/*-------------------------------------------------------------------------
 - raytracing / visibility related stuff
 -------------------------------------------------------------------------*/
struct ray {
  INLINE ray(void) {}
  INLINE ray(vec3f org, vec3f dir, float near = 0.f, float far = FLT_MAX)
    : org(org), dir(dir), tnear(near), tfar(far) {}
  vec3f org, dir;
  float tnear, tfar;
};

struct CACHE_LINE_ALIGNED raypacket {
  static const u32 MAXRAYNUM     = 256;
  static const u32 COMMONORG     = 1<<0;
  static const u32 COMMONDIR     = 1<<1;
  static const u32 INTERVALARITH = 1<<2;
  static const u32 CORNERRAYS    = 1<<3;
  INLINE raypacket(void) : raynum(0), flags(0) {}
  INLINE void setorg(vec3f org, u32 rayid) {
    orgx[rayid] = org.x;
    orgy[rayid] = org.y;
    orgz[rayid] = org.z;
  }
  INLINE void setdir(vec3f dir, u32 rayid) {
    dirx[rayid] = dir.x;
    diry[rayid] = dir.y;
    dirz[rayid] = dir.z;
  }
  INLINE vec3f org(u32 rayid=0) const {
    return vec3f(orgx[rayid], orgy[rayid], orgz[rayid]);
  }
  INLINE vec3f dir(u32 rayid=0) const {
    return vec3f(dirx[rayid], diry[rayid], dirz[rayid]);
  }
  array<float,MAXRAYNUM> orgx, orgy, orgz;
  array<float,MAXRAYNUM> dirx, diry, dirz;
  interval3f iaorg, iadir, iardir;
  u32 raynum;
  u32 flags;
};

struct camera {
  camera(vec3f org, vec3f up, vec3f view, float fov, float ratio);
  INLINE ray generate(int w, int h, int x, int y) const {
    const float rw = rcp(float(w)), rh = rcp(float(h));
    const vec3f sxaxis = xaxis*rw, szaxis = zaxis*rh;
    const vec3f dir = normalize(imgplaneorg + float(x)*sxaxis + float(y)*szaxis);
    return ray(org, dir);
  }
  vec3f org, up, view, imgplaneorg, xaxis, zaxis;
  float fov, ratio, dist;
};

struct aabb {
  INLINE aabb(void) {}
  INLINE aabb(float m, float M) : pmin(m), pmax(M) {}
  INLINE aabb(vec3f m, vec3f M) : pmin(m), pmax(M) {}
  INLINE void compose(const aabb &other) {
    pmin = min(pmin, other.pmin);
    pmax = max(pmax, other.pmax);
  }
  INLINE float halfarea(void) const {
    const vec3f e(pmax-pmin);
    return e.x*e.y + e.y*e.z + e.x*e.z;
  }
  static INLINE aabb empty() { return aabb(FLT_MAX, -FLT_MAX); }
  static INLINE aabb all() { return aabb(-FLT_MAX, FLT_MAX); }
  vec3f pmin, pmax;
};
INLINE aabb sum(const aabb &b0, const aabb &b1) {
  return aabb(min(b0.pmin, b1.pmin), max(b0.pmax, b1.pmax));
}
INLINE aabb intersection(const aabb &b0, const aabb &b1) {
  return aabb(max(b0.pmin, b1.pmin), min(b0.pmax, b1.pmax));
}
INLINE bool intersect(const aabb &b0, const aabb &b1) {
  return !(any(gt(b0.pmin, b1.pmax)) || any(gt(b1.pmin, b0.pmax)));
}

struct isecres {
  INLINE isecres(bool isec, float t = FLT_MAX) : t(t), isec(isec) {}
  float t;
  bool isec;
};

INLINE isecres slab(vec3f pmin, vec3f pmax, vec3f rdir, float t) {
  const vec3f l1 = pmin*rdir;
  const vec3f l2 = pmax*rdir;
  const float tfar = reducemin(max(l1,l2));
  const float tnear = reducemax(min(l1,l2));
  return isecres((tfar >= tnear) & (tfar >= 0.f) & (tnear < t), max(0.f,tnear));
}

INLINE isecres slab(const aabb &box, vec3f org, vec3f rdir, float t) {
  return slab(box.pmin-org,box.pmax-org, rdir, t);
}

} /* namespace q */

