/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - soa.hpp -> factorizes code shared by all simd paths
 -------------------------------------------------------------------------*/
#pragma once
#include "base/avx.hpp"
#include "base/sse.hpp"
#include "base/math.hpp"

namespace q {

/*-------------------------------------------------------------------------
 - define soa structures based on the native ISA we use
 -------------------------------------------------------------------------*/
#if defined(__AVX__)
typedef avxf soaf;
typedef avxi soai;
typedef avxb soab;
INLINE void store(void *ptr, const soai &x) {store8i(ptr, x);}
INLINE void store(void *ptr, const soaf &x) {store8f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store8b(ptr, x);}
INLINE void storeu(void *ptr, const soaf &x) {storeu8f(ptr, x);}
INLINE void storeui(void *ptr, const soai &x) {storeu8i(ptr, x);}
INLINE void storent(void *ptr, const soai &x) {store8i_nt(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store8f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {
  const auto p = shuffle<0,0>(v);
  const auto s = shuffle<0,0,0,0>(p,p);
  return s;
}
#elif defined(__SSE__)
typedef ssef soaf;
typedef ssei soai;
typedef sseb soab;
INLINE void store(void *ptr, const soai &x) {store4i(ptr, x);}
INLINE void store(void *ptr, const soaf &x) {store4f(ptr, x);}
INLINE void store(void *ptr, const soab &x) {store4b(ptr, x);}
INLINE void storent(void *ptr, const soai &x) {store4i_nt(ptr, x);}
INLINE void storeu(void *ptr, const soaf &x) {storeu4f(ptr, x);}
INLINE void storeui(void *ptr, const soai &x) {storeu4i(ptr, x);}
INLINE void maskstore(const soab &m, void *ptr, const soaf &x) {store4f(m, ptr, x);}
INLINE soaf splat(const soaf &v) {return shuffle<0,0,0,0>(v,v);}
#else
#error "unsupported SIMD"
#endif

typedef vec3<soai> soa3i;
typedef vec3<soaf> soa3f;
typedef vec2<soaf> soa2f;

/*-------------------------------------------------------------------------
 - define soa structures based on array which are oblivious to the
 - underlying simd ISA. they are making easy interoperability regarldess of
 - ISA we use
 -------------------------------------------------------------------------*/
template <int n> using arrayi = array<int,n>;
template <int n> using arrayf = array<float,n>;
template <int n> using array2f = vec2<arrayf<n>>;
template <int n> using array3f = vec3<arrayf<n>>;
template <int n> using array4f = vec4<arrayf<n>>;

template <int n>
INLINE soaf sget(const arrayf<n> &x, u32 idx) {
  return soaf::load(&x[idx*soaf::size]);
}

template <int n>
INLINE soa3f sget(const array3f<n> &v, u32 idx) {
  const soaf x = sget(v[0], idx);
  const soaf y = sget(v[1], idx);
  const soaf z = sget(v[2], idx);
  return soa3f(x,y,z);
}

template <int n>
INLINE void sset(array3f<n> &out, const soa3f &v, u32 idx) {
  store(&out[0][idx*soaf::size], v.x);
  store(&out[1][idx*soaf::size], v.y);
  store(&out[2][idx*soaf::size], v.z);
}
} /* namespace q */

