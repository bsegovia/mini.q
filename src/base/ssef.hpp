/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ssef.hpp -> implements FP operations with SSE vectors
 -------------------------------------------------------------------------*/
// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //
#pragma once
#include "sys.hpp"
#include "math.hpp"

#define op operator
namespace q {

/*-------------------------------------------------------------------------
 - 4-wide SSE float type
 -------------------------------------------------------------------------*/
struct ssef {
  typedef struct sseb masktype;  // mask type
  typedef struct ssei inttype;   // int type
  typedef struct ssef floattype; // float type

  enum   {size = 4};  // number of SIMD elements
  union {__m128 m128; float f[4]; int i[4];}; // data

  // constructors, assignment & cast ops
  INLINE ssef() {}
  INLINE ssef(const ssef& other) {m128 = other.m128;}
  INLINE ssef& op=(const ssef& other) {m128 = other.m128; return *this;}
  INLINE ssef(const __m128 a) : m128(a) {}
  INLINE op const __m128&(void) const {return m128;}
  INLINE op       __m128&(void)       {return m128;}
  INLINE ssef(float a) : m128(_mm_set1_ps(a)) {}
  INLINE ssef(float a, float b) : m128(_mm_set_ps(b, a, b, a)) {}
  INLINE ssef(float a, float b, float c, float d) : m128(_mm_set_ps(d, c, b, a)) {}
  INLINE explicit ssef(const __m128i a) : m128(_mm_cvtepi32_ps(a)) {}

  // constants
  INLINE ssef(zerotype) : m128(_mm_setzero_ps()) {}
  INLINE ssef(onetype) : m128(_mm_set1_ps(1.0f)) {}

  // loads and stores
#if defined(__AVX__)
  static INLINE ssef broadcast(const void* const a) {return _mm_broadcast_ss((float*)a);}
#else
  static INLINE ssef broadcast(const void* const a) {return _mm_set1_ps(*(float*)a);}
#endif
  static INLINE ssef load(const void* const ptr) {return *(__m128*)ptr;}

  // array access
  INLINE const float& op [](const size_t i) const {assert(i<4); return f[i];}
  INLINE       float& op [](const size_t i)       {assert(i<4); return f[i];}
};

// unary ops
INLINE ssef cast(const __m128i& a) {return _mm_castsi128_ps(a);}
INLINE ssef op +(const ssef& a) {return a;}
INLINE ssef op -(const ssef& a) {
  return _mm_xor_ps(a.m128, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
}
INLINE ssef abs(const ssef& a) {
  return _mm_and_ps(a.m128, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
}
#if defined(__SSE4_1__)
INLINE ssef sign(const ssef& a) {
  return _mm_blendv_ps(ssef(one), -ssef(one), _mm_cmplt_ps (a,ssef(zero)));
}
#endif
INLINE ssef signmsk(const ssef& a) {
  return _mm_and_ps(a.m128,_mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
}

INLINE ssef rcp  (const ssef& a) {
  const ssef r = _mm_rcp_ps(a.m128);
  return _mm_sub_ps(_mm_add_ps(r, r), _mm_mul_ps(_mm_mul_ps(r, r), a));
}
INLINE ssef sqr  (const ssef& a) {return _mm_mul_ps(a,a);}
INLINE ssef sqrt (const ssef& a) {return _mm_sqrt_ps(a.m128);}
INLINE ssef rsqrt(const ssef& a) {
  const ssef r = _mm_rsqrt_ps(a.m128);
  return _mm_add_ps(_mm_mul_ps(_mm_set_ps(1.5f, 1.5f, 1.5f, 1.5f), r),
                    _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a,
                     _mm_set_ps(-0.5f, -0.5f, -0.5f, -0.5f)), r), _mm_mul_ps(r, r)));
}

// binary ops
INLINE ssef op +(const ssef& a, const ssef& b) {return _mm_add_ps(a.m128, b.m128);}
INLINE ssef op +(const ssef& a, const float& b) {return a + ssef(b);}
INLINE ssef op +(const float& a, const ssef& b) {return ssef(a) + b;}
INLINE ssef op -(const ssef& a, const ssef& b) {return _mm_sub_ps(a.m128, b.m128);}
INLINE ssef op -(const ssef& a, const float& b) {return a - ssef(b);}
INLINE ssef op -(const float& a, const ssef& b) {return ssef(a) - b;}
INLINE ssef op *(const ssef& a, const ssef& b) {return _mm_mul_ps(a.m128, b.m128);}
INLINE ssef op *(const ssef& a, const float& b) {return a * ssef(b);}
INLINE ssef op *(const float& a, const ssef& b) {return ssef(a) * b;}
INLINE ssef op /(const ssef& a, const ssef& b) {return _mm_div_ps(a.m128,b.m128);}
INLINE ssef op /(const ssef& a, const float& b) {return a/ssef(b);}
INLINE ssef op /(const float& a, const ssef& b) {return ssef(a)/b;}
INLINE ssef op^(const ssef& a, const ssef& b) {return _mm_xor_ps(a.m128,b.m128);}
INLINE ssef op^(const ssef& a, const ssei& b) {return _mm_xor_ps(a.m128,_mm_castsi128_ps(b.m128));}
INLINE ssef min(const ssef& a, const ssef& b) {return _mm_min_ps(a.m128,b.m128);}
INLINE ssef min(const ssef& a, const float& b) {return _mm_min_ps(a.m128,ssef(b));}
INLINE ssef min(const float& a, const ssef& b) {return _mm_min_ps(ssef(a),b.m128);}
INLINE ssef max(const ssef& a, const ssef& b) {return _mm_max_ps(a.m128,b.m128);}
INLINE ssef max(const ssef& a, const float& b) {return _mm_max_ps(a.m128,ssef(b));}
INLINE ssef max(const float& a, const ssef& b) {return _mm_max_ps(ssef(a),b.m128);}

#if defined(__SSE4_1__)
INLINE ssef mini(const ssef& a, const ssef& b) {
  const ssei ai = _mm_castps_si128(a);
  const ssei bi = _mm_castps_si128(b);
  const ssei ci = _mm_min_epi32(ai,bi);
  return _mm_castsi128_ps(ci);
}
#endif

#if defined(__SSE4_1__)
INLINE ssef maxi(const ssef& a, const ssef& b) {
  const ssei ai = _mm_castps_si128(a);
  const ssei bi = _mm_castps_si128(b);
  const ssei ci = _mm_max_epi32(ai,bi);
  return _mm_castsi128_ps(ci);
}
#endif

// ternary ops
#if defined(__AVX2__)
INLINE ssef madd (const ssef& a, const ssef& b, const ssef& c) {return _mm_fmadd_ps(a,b,c);}
INLINE ssef msub (const ssef& a, const ssef& b, const ssef& c) {return _mm_fmsub_ps(a,b,c);}
INLINE ssef nmadd(const ssef& a, const ssef& b, const ssef& c) {return _mm_fnmadd_ps(a,b,c);}
INLINE ssef nmsub(const ssef& a, const ssef& b, const ssef& c) {return _mm_fnmsub_ps(a,b,c);}
#else
INLINE ssef madd (const ssef& a, const ssef& b, const ssef& c) {return a*b+c;}
INLINE ssef msub (const ssef& a, const ssef& b, const ssef& c) {return a*b-c;}
INLINE ssef nmadd(const ssef& a, const ssef& b, const ssef& c) {return -a*b-c;}
INLINE ssef nmsub(const ssef& a, const ssef& b, const ssef& c) {return c-a*b;}
#endif

// assignment ops
INLINE ssef &op+= (ssef& a, const ssef& b) {return a = a + b;}
INLINE ssef &op+= (ssef& a, const float& b) {return a = a + b;}
INLINE ssef &op-= (ssef& a, const ssef& b) {return a = a - b;}
INLINE ssef &op-= (ssef& a, const float& b) {return a = a - b;}
INLINE ssef &op*= (ssef& a, const ssef& b) {return a = a * b;}
INLINE ssef &op*= (ssef& a, const float& b) {return a = a * b;}
INLINE ssef &op/= (ssef& a, const ssef& b) {return a = a / b;}
INLINE ssef &op/= (ssef& a, const float& b) {return a = a / b;}

// comparison ops + select
INLINE sseb op== (const ssef& a, const ssef& b) {return _mm_cmpeq_ps (a.m128, b.m128);}
INLINE sseb op== (const ssef& a, const float& b) {return a == ssef(b);}
INLINE sseb op== (const float& a, const ssef& b) {return ssef(a) == b;}
INLINE sseb op!= (const ssef& a, const ssef& b) {return _mm_cmpneq_ps(a.m128, b.m128);}
INLINE sseb op!= (const ssef& a, const float& b) {return a != ssef(b);}
INLINE sseb op!= (const float& a, const ssef& b) {return ssef(a) != b;}
INLINE sseb op<  (const ssef& a, const ssef& b) {return _mm_cmplt_ps (a.m128, b.m128);}
INLINE sseb op<  (const ssef& a, const float& b) {return a <  ssef(b);}
INLINE sseb op<  (const float& a, const ssef& b) {return ssef(a) <  b;}
INLINE sseb op>= (const ssef& a, const ssef& b) {return _mm_cmpnlt_ps(a.m128, b.m128);}
INLINE sseb op>= (const ssef& a, const float& b) {return a >= ssef(b);}
INLINE sseb op>= (const float& a, const ssef& b) {return ssef(a) >= b;}
INLINE sseb op>  (const ssef& a, const ssef& b) {return _mm_cmpnle_ps(a.m128, b.m128);}
INLINE sseb op>  (const ssef& a, const float& b) {return a >  ssef(b);}
INLINE sseb op>  (const float& a, const ssef& b) {return ssef(a) >  b;}
INLINE sseb op<= (const ssef& a, const ssef& b) {return _mm_cmple_ps (a.m128, b.m128);}
INLINE sseb op<= (const ssef& a, const float& b) {return a <= ssef(b);}
INLINE sseb op<= (const float& a, const ssef& b) {return ssef(a) <= b;}

INLINE ssef select(const sseb& m, const ssef& t, const ssef& f) {
#if defined(__SSE4_1__)
  return _mm_blendv_ps(f, t, m);
#else
  return _mm_or_ps(_mm_and_ps(m, t), _mm_andnot_ps(m, f));
#endif
}

#if defined(__SSE4_1__) && !defined(__clang__)
INLINE ssef select(const int mask, const ssef& t, const ssef& f) {
  return _mm_blend_ps(f, t, mask);
}
#else
INLINE ssef select(const int mask, const ssef& t, const ssef& f) {
  return select(sseb(mask),t,f);
}
#endif

// rounding functions
#if defined (__SSE4_1__)
INLINE ssef round_even(const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_NEAREST_INT);}
INLINE ssef round_down(const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF);}
INLINE ssef round_up  (const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_POS_INF);}
INLINE ssef round_zero(const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_ZERO);}
INLINE ssef floor     (const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_NEG_INF);}
INLINE ssef ceil      (const ssef& a) {return _mm_round_ps(a, _MM_FROUND_TO_POS_INF);}
#endif

INLINE ssei floori (const ssef& a) {
#if defined (__SSE4_1__)
  return ssei(floor(a));
#else
  return ssei(a-ssef(0.5f));
#endif
}

// movement/shifting/shuffling functions
INLINE ssef unpacklo(const ssef& a, const ssef& b) {return _mm_unpacklo_ps(a.m128, b.m128);}
INLINE ssef unpackhi(const ssef& a, const ssef& b) {return _mm_unpackhi_ps(a.m128, b.m128);}
INLINE ssef andnot(const ssef& a, const ssef& b) {return _mm_andnot_ps(a.m128, b.m128);}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE ssef shuffle(const ssef& b) {
  return _mm_castsi128_ps(
    _mm_shuffle_epi32(_mm_castps_si128(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE ssef shuffle(const ssef& a, const ssef& b) {
  return _mm_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

#if defined (__SSSE3__)
INLINE ssef shuffle8(const ssef& a, const ssei& shuf) {
  return _mm_castsi128_ps(_mm_shuffle_epi8(_mm_castps_si128(a), shuf));
}
#endif

#if defined(__SSE3__)
template<>
INLINE ssef shuffle<0, 0, 2, 2>(const ssef& b) {return _mm_moveldup_ps(b);}
template<>
INLINE ssef shuffle<1, 1, 3, 3>(const ssef& b) {return _mm_movehdup_ps(b);}
template<>
INLINE ssef shuffle<0, 1, 0, 1>(const ssef& b) {
  return _mm_castpd_ps(_mm_movedup_pd(_mm_castps_pd(b)));
}
#endif
template<size_t i0> INLINE ssef shuffle(const ssef& b) {
  return shuffle<i0,i0,i0,i0>(b);
}

#if defined (__SSE4_1__) && !defined(__GNUC__)
template<size_t i> INLINE float extract   (const ssef& a) {return _mm_cvtss_f32(_mm_extract_ps(a,i));}
#else
template<size_t i> INLINE float extract   (const ssef& a) {return _mm_cvtss_f32(shuffle<i,i,i,i>(a));}
#endif
template<>         INLINE float extract<0>(const ssef& a) {return _mm_cvtss_f32(a);}

#if defined (__SSE4_1__)
template<size_t dst, size_t src, size_t clr>
INLINE ssef insert(const ssef& a, const ssef& b) {
  return _mm_insert_ps(a, b, (dst << 4) | (src << 6) | clr);
}
template<size_t dst, size_t src>
INLINE ssef insert(const ssef& a, const ssef& b) {
  return insert<dst, src, 0>(a, b);
}
template<size_t dst>
INLINE ssef insert(const ssef& a, float b) {
  return insert<dst,0>(a, _mm_set_ss(b));
}
#else
template<size_t dst>
INLINE const ssef insert(const ssef& a, const float b) {
  ssef c = a; c[dst] = b; return c;
}
#endif

// transpose
INLINE void transpose(const ssef& r0, const ssef& r1, const ssef& r2, const ssef& r3,
                      ssef& c0, ssef& c1, ssef& c2, ssef& c3) {
  ssef l02 = unpacklo(r0,r2);
  ssef h02 = unpackhi(r0,r2);
  ssef l13 = unpacklo(r1,r3);
  ssef h13 = unpackhi(r1,r3);
  c0 = unpacklo(l02,l13);
  c1 = unpackhi(l02,l13);
  c2 = unpacklo(h02,h13);
  c3 = unpackhi(h02,h13);
}

INLINE void transpose(const ssef& r0, const ssef& r1, const ssef& r2, const ssef& r3,
                      ssef& c0, ssef& c1, ssef& c2) {
  ssef l02 = unpacklo(r0,r2);
  ssef h02 = unpackhi(r0,r2);
  ssef l13 = unpacklo(r1,r3);
  ssef h13 = unpackhi(r1,r3);
  c0 = unpacklo(l02,l13);
  c1 = unpackhi(l02,l13);
  c2 = unpacklo(h02,h13);
}

// reductions
INLINE ssef vreduce_min(const ssef& v) {
  ssef h = min(shuffle<1,0,3,2>(v),v);
  return min(shuffle<2,3,0,1>(h),h);
}
INLINE ssef vreduce_max(const ssef& v) {
  ssef h = max(shuffle<1,0,3,2>(v),v);
  return max(shuffle<2,3,0,1>(h),h);
}
INLINE ssef vreduce_add(const ssef& v) {
  ssef h = shuffle<1,0,3,2>(v)+v;
  return shuffle<2,3,0,1>(h)+h;
}
INLINE float reduce_min(const ssef& v) {return _mm_cvtss_f32(vreduce_min(v));}
INLINE float reduce_max(const ssef& v) {return _mm_cvtss_f32(vreduce_max(v));}
INLINE float reduce_add(const ssef& v) {return _mm_cvtss_f32(vreduce_add(v));}
INLINE size_t select_min(const ssef& v) {return __bsf(movemask(v == vreduce_min(v)));}
INLINE size_t select_max(const ssef& v) {return __bsf(movemask(v == vreduce_max(v)));}

// memory load and store operations
INLINE ssef load4f(const void* const a) {return _mm_load_ps((float*)a);}
INLINE void store4f (void* ptr, const ssef& v) {_mm_store_ps((float*)ptr,v);}
INLINE ssef loadu4f(const void* const a) {return _mm_loadu_ps((float*)a);}
INLINE void storeu4f (void* ptr, const ssef& v) {_mm_storeu_ps((float*)ptr,v);}

INLINE void store4f (const sseb& mask, void* ptr, const ssef& f) {
#if defined (__AVX__)
  _mm_maskstore_ps((float*)ptr,(__m128i)mask,f);
#else
  *(ssef*)ptr = select(mask,f,*(ssef*)ptr);
#endif
}

INLINE ssef load4f_nt (void* ptr) {
#if defined (__SSE4_1__)
  return _mm_castsi128_ps(_mm_stream_load_si128((__m128i*)ptr));
#else
  return _mm_load_ps((float*)ptr);
#endif
}

INLINE void store4f_nt (void* ptr, const ssef& v) {
#if defined (__SSE4_1__)
  _mm_stream_ps((float*)ptr,v);
#else
  _mm_store_ps((float*)ptr,v);
#endif
}

// euclidian space ops
INLINE float dot (const ssef& a, const ssef& b) {
  return reduce_add(a*b);
}

INLINE ssef cross(const ssef& a, const ssef& b) {
  const ssef a0 = a;
  const ssef b0 = shuffle<1,2,0,3>(b);
  const ssef a1 = shuffle<1,2,0,3>(a);
  const ssef b1 = b;
  return shuffle<1,2,0,3>(msub(a0,b0,a1*b1));
}
} /* namespace q */
#undef op

