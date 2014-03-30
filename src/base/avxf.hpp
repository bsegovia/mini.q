/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - avxf.hpp -> implements FP operations with AVX vectors
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

#define op operator
namespace q {

/*-------------------------------------------------------------------------
 - 8-wide AVX float type
 -------------------------------------------------------------------------*/
struct avxf {
  typedef struct avxb masktype; // mask type for us
  typedef struct avxi inttype;  // int type for us

  enum {size = 8}; // number of SIMD elements
  union {__m256 m256; float v[8];}; // data

  // constructors, assignment & cast operators
  INLINE avxf() {}
  INLINE avxf(const avxf& other) {m256 = other.m256;}
  INLINE avxf& op=(const avxf& other) {m256 = other.m256; return *this;}
  INLINE avxf(__m256 a) : m256(a) {}
  INLINE op const __m256&(void) const {return m256;}
  INLINE op __m256&(void) {return m256;}
  INLINE explicit avxf(const ssef &a) :
    m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),a,1)) {}
  INLINE avxf(const ssef &a, const ssef &b) :
    m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),b,1)) {}
  static INLINE avxf load(const void* const ptr) {return *(__m256*)ptr;}
  INLINE explicit avxf(const char* const a) : m256(_mm256_loadu_ps((const float*)a)) {}
  INLINE avxf(const float &a) : m256(_mm256_broadcast_ss(&a)) {}
  INLINE avxf(float a, float b) : m256(_mm256_set_ps(b, a, b, a, b, a, b, a)) {}
  INLINE avxf(float a, float b, float c, float d) :
    m256(_mm256_set_ps(d, c, b, a, d, c, b, a)) {}
  INLINE avxf(float a, float b, float c, float d, float e, float f, float g, float h) :
    m256(_mm256_set_ps(h, g, f, e, d, c, b, a)) {}
  INLINE explicit avxf(const __m256i a) : m256(_mm256_cvtepi32_ps(a)) {}

  // constants
  INLINE avxf(zerotype) : m256(_mm256_setzero_ps()) {}
  INLINE avxf(onetype) : m256(_mm256_set1_ps(1.0f)) {}
  static INLINE avxf broadcast(const void* const a) {
    return _mm256_broadcast_ss((float*)a);
 }

  // array access
  INLINE const float& op [](const size_t i) const {assert(i < 8); return v[i];}
  INLINE       float& op [](const size_t i)       {assert(i < 8); return v[i];}
};

// unary operators
INLINE avxf cast(const __m256i& a) {return _mm256_castsi256_ps(a);}
INLINE avxf cast(const avxi& a) {return _mm256_castsi256_ps(a);}
INLINE avxi cast(const avxf& a) {return _mm256_castps_si256(a);}
INLINE avxf op +(const avxf& a) {return a;}
INLINE avxf op -(const avxf& a) {
  const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000));
  return _mm256_xor_ps(a.m256, mask);
}
INLINE avxf abs(const avxf& a) {
  const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff));
  return _mm256_and_ps(a.m256, mask);
}
INLINE avxf sign(const avxf& a) {
  return _mm256_blendv_ps(avxf(one), -avxf(one), _mm256_cmp_ps(a, avxf(zero), _CMP_NGE_UQ));
}
INLINE avxf signmsk(const avxf& a) {
  return _mm256_and_ps(a.m256,_mm256_castsi256_ps(_mm256_set1_epi32(0x80000000)));
}

INLINE avxf rcp(const avxf& a) { return _mm256_div_ps(avxf(one), a); }
INLINE avxf sqr  (const avxf& a) {return _mm256_mul_ps(a,a);}
INLINE avxf sqrt (const avxf& a) {return _mm256_sqrt_ps(a.m256);}
INLINE avxf rsqrt(const avxf& a) {
  const avxf r = _mm256_rsqrt_ps(a.m256);
  return _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.5f), r),
    _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(a, _mm256_set1_ps(-0.5f)), r),
      _mm256_mul_ps(r, r)));
}

// binary operators
INLINE avxf op+ (const avxf& a, const avxf& b) {return _mm256_add_ps(a.m256, b.m256);}
INLINE avxf op+ (const avxf& a, const float b) {return a + avxf(b);}
INLINE avxf op+ (const float a, const avxf& b) {return avxf(a) + b;}
INLINE avxf op- (const avxf& a, const avxf& b) {return _mm256_sub_ps(a.m256, b.m256);}
INLINE avxf op- (const avxf& a, const float b) {return a - avxf(b);}
INLINE avxf op- (const float a, const avxf& b) {return avxf(a) - b;}
INLINE avxf op* (const avxf& a, const avxf& b) {return _mm256_mul_ps(a.m256, b.m256);}
INLINE avxf op* (const avxf& a, const float b) {return a * avxf(b);}
INLINE avxf op* (const float a, const avxf& b) {return avxf(a) * b;}
INLINE avxf op/ (const avxf& a, const avxf& b) {return _mm256_div_ps(a.m256, b.m256);}
INLINE avxf op/ (const avxf& a, const float b) {return a / avxf(b);}
INLINE avxf op/ (const float a, const avxf& b) {return avxf(a) / b;}
INLINE avxf op^ (const avxf& a, const avxf& b) {return _mm256_xor_ps(a.m256,b.m256);}
INLINE avxf op^ (const avxf& a, const avxi& b) {return _mm256_xor_ps(a.m256,_mm256_castsi256_ps(b.m256));}
INLINE avxf op&(const avxf& a, const avxf& b) {return _mm256_and_ps(a.m256,b.m256);}
INLINE avxf min(const avxf& a, const avxf& b) {return _mm256_min_ps(a.m256, b.m256);}
INLINE avxf min(const avxf& a, const float b) {return _mm256_min_ps(a.m256, avxf(b));}
INLINE avxf min(const float a, const avxf& b) {return _mm256_min_ps(avxf(a), b.m256);}
INLINE avxf max(const avxf& a, const avxf& b) {return _mm256_max_ps(a.m256, b.m256);}
INLINE avxf max(const avxf& a, const float b) {return _mm256_max_ps(a.m256, avxf(b));}
INLINE avxf max(const float a, const avxf& b) {return _mm256_max_ps(avxf(a), b.m256);}

#if defined (__AVX2__)
INLINE avxf mini(const avxf& a, const avxf& b) {
  const avxi ai = _mm256_castps_si256(a);
  const avxi bi = _mm256_castps_si256(b);
  const avxi ci = _mm256_min_epi32(ai,bi);
  return _mm256_castsi256_ps(ci);
}
INLINE avxf maxi(const avxf& a, const avxf& b) {
  const avxi ai = _mm256_castps_si256(a);
  const avxi bi = _mm256_castps_si256(b);
  const avxi ci = _mm256_max_epi32(ai,bi);
  return _mm256_castsi256_ps(ci);
}
#endif

// ternary operators
#if defined(__AVX2__)
INLINE avxf madd(const avxf& a, const avxf& b, const avxf& c) {return _mm256_fmadd_ps(a,b,c);}
INLINE avxf msub(const avxf& a, const avxf& b, const avxf& c) {return _mm256_fmsub_ps(a,b,c);}
INLINE avxf nmadd(const avxf& a, const avxf& b, const avxf& c) {return _mm256_fnmadd_ps(a,b,c);}
INLINE avxf nmsub(const avxf& a, const avxf& b, const avxf& c) {return _mm256_fnmsub_ps(a,b,c);}
#else
INLINE avxf madd(const avxf& a, const avxf& b, const avxf& c) {return a*b+c;}
INLINE avxf msub(const avxf& a, const avxf& b, const avxf& c) {return a*b-c;}
INLINE avxf nmadd(const avxf& a, const avxf& b, const avxf& c) {return -a*b-c;}
INLINE avxf nmsub(const avxf& a, const avxf& b, const avxf& c) {return c-a*b;}
#endif

// assignment operators
INLINE avxf &op+= (avxf& a, const avxf& b) {return a = a + b;}
INLINE avxf &op+= (avxf& a, const float b) {return a = a + b;}
INLINE avxf &op-= (avxf& a, const avxf& b) {return a = a - b;}
INLINE avxf &op-= (avxf& a, const float b) {return a = a - b;}
INLINE avxf &op*= (avxf& a, const avxf& b) {return a = a * b;}
INLINE avxf &op*= (avxf& a, const float b) {return a = a * b;}
INLINE avxf &op/= (avxf& a, const avxf& b) {return a = a / b;}
INLINE avxf &op/= (avxf& a, const float b) {return a = a / b;}

/// comparison operators + select
INLINE avxb op== (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_EQ_OQ);}
INLINE avxb op== (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_EQ_OQ);}
INLINE avxb op== (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_EQ_OQ);}
INLINE avxb op!= (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_NEQ_OQ);}
INLINE avxb op!= (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_NEQ_OQ);}
INLINE avxb op!= (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_NEQ_OQ);}
INLINE avxb op<  (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_LT_OQ);}
INLINE avxb op<  (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_LT_OQ);}
INLINE avxb op<  (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_LT_OQ);}
INLINE avxb op>= (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_GE_OQ);}
INLINE avxb op>= (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_GE_OQ);}
INLINE avxb op>= (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_GE_OQ);}
INLINE avxb op>  (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_GT_OQ);}
INLINE avxb op>  (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_GT_OQ);}
INLINE avxb op>  (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_GT_OQ);}
INLINE avxb op<= (const avxf& a, const avxf& b) {return _mm256_cmp_ps(a.m256, b.m256, _CMP_LE_OQ);}
INLINE avxb op<= (const avxf& a, const float b) {return _mm256_cmp_ps(a.m256, avxf(b), _CMP_LE_OQ);}
INLINE avxb op<= (const float a, const avxf& b) {return _mm256_cmp_ps(avxf(a), b.m256, _CMP_LE_OQ);}

INLINE avxf select(const avxb& m, const avxf& t, const avxf& f) {
  return _mm256_blendv_ps(f, t, m);
}

#if !defined(__clang__)
INLINE avxf select(const int m, const avxf& t, const avxf& f) {
  return _mm256_blend_ps(f, t, m);
}
#else
INLINE avxf select(const int m, const avxf& t, const avxf& f) {
  return select(avxb(m),t,f);
}
#endif

// rounding functions
INLINE avxf round_even(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_NEAREST_INT);}
INLINE avxf round_down(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF);}
INLINE avxf round_up(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF);}
INLINE avxf round_zero(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_ZERO   );}
INLINE avxf floor(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_NEG_INF);}
INLINE avxf ceil(const avxf& a) {return _mm256_round_ps(a, _MM_FROUND_TO_POS_INF);}

// movement/shifting/shuffling functions
INLINE avxf unpacklo(const avxf& a, const avxf& b) {return _mm256_unpacklo_ps(a.m256, b.m256);}
INLINE avxf unpackhi(const avxf& a, const avxf& b) {return _mm256_unpackhi_ps(a.m256, b.m256);}

template<size_t i>
INLINE avxf shuffle(const avxf& a) {
  return _mm256_permute_ps(a, _MM_SHUFFLE(i, i, i, i));
}
template<size_t i0, size_t i1>
INLINE avxf shuffle(const avxf& a) {
  return _mm256_permute2f128_ps(a, a, (i1 << 4) | (i0 << 0));
}
template<size_t i0, size_t i1>
INLINE avxf shuffle(const avxf& a,  const avxf& b) {
  return _mm256_permute2f128_ps(a, b, (i1 << 4) | (i0 << 0));
}
template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE avxf shuffle(const avxf& a) {
  return _mm256_permute_ps(a, _MM_SHUFFLE(i3, i2, i1, i0));
}
template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE avxf shuffle(const avxf& a, const avxf& b) {
  return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

template<>
INLINE avxf shuffle<0, 0, 2, 2>(const avxf& b) {
  return _mm256_moveldup_ps(b);
}
template<> INLINE
avxf shuffle<1, 1, 3, 3>(const avxf& b) {
  return _mm256_movehdup_ps(b);
}
template<> INLINE
avxf shuffle<0, 1, 0, 1>(const avxf& b) {
  return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(b)));
}

INLINE avxf broadcast(const float* ptr) {return _mm256_broadcast_ss(ptr);}
template<size_t i>
INLINE avxf insert(const avxf& a, const ssef& b) {
  return _mm256_insertf128_ps (a,b,i);
}
template<size_t i>
INLINE ssef extract(const avxf &a) {
  return _mm256_extractf128_ps(a, i);
}
template<>
INLINE ssef extract<0>(const avxf &a) {
  return _mm256_castps256_ps128(a);
}

template<size_t i>
INLINE float fextract(const avxf &a) {
  return _mm_cvtss_f32(_mm256_extractf128_ps(a, i));
}

#if defined (__AVX2__)
INLINE avxf permute(const avxf &a, const __m256i &index) {
  return _mm256_permutevar8x32_ps(a,index);
}

template<int i>
INLINE avxf alignr(const avxf &a, const avxf &b) {
  return _mm256_castsi256_ps(
    _mm256_alignr_epi8(_mm256_castps_si256(a), _mm256_castps_si256(b), i));
}

template<const int mode>
INLINE ssei convert_to_hf16(const avxf &a) {
  return _mm256_cvtps_ph(a,mode);
}

INLINE avxf convert_from_hf16(const ssei &a) {
  return _mm256_cvtph_ps(a);
}
#endif

// transpose
INLINE void transpose4(const avxf& r0, const avxf& r1, const avxf& r2, const avxf& r3,
                       avxf& c0, avxf& c1, avxf& c2, avxf& c3) {
  avxf l02 = unpacklo(r0,r2);
  avxf h02 = unpackhi(r0,r2);
  avxf l13 = unpacklo(r1,r3);
  avxf h13 = unpackhi(r1,r3);
  c0 = unpacklo(l02,l13);
  c1 = unpackhi(l02,l13);
  c2 = unpacklo(h02,h13);
  c3 = unpackhi(h02,h13);
}

INLINE void transpose(const avxf& r0, const avxf& r1, const avxf& r2, const avxf& r3,
                      const avxf& r4, const avxf& r5, const avxf& r6, const avxf& r7,
                      avxf& c0, avxf& c1, avxf& c2, avxf& c3,
                      avxf& c4, avxf& c5, avxf& c6, avxf& c7) {
  avxf h0,h1,h2,h3;
  avxf h4,h5,h6,h7;
  transpose4(r0,r1,r2,r3,h0,h1,h2,h3);
  transpose4(r4,r5,r6,r7,h4,h5,h6,h7);
  c0 = shuffle<0,2>(h0,h4);
  c1 = shuffle<0,2>(h1,h5);
  c2 = shuffle<0,2>(h2,h6);
  c3 = shuffle<0,2>(h3,h7);
  c4 = shuffle<1,3>(h0,h4);
  c5 = shuffle<1,3>(h1,h5);
  c6 = shuffle<1,3>(h2,h6);
  c7 = shuffle<1,3>(h3,h7);
}

// reductions
INLINE avxf vreduce_min2(const avxf& v) {return min(v,shuffle<1,0,3,2>(v));}
INLINE avxf vreduce_min4(const avxf& v) {avxf v1 = vreduce_min2(v); return min(v1,shuffle<2,3,0,1>(v1));}
INLINE avxf vreduce_min (const avxf& v) {avxf v1 = vreduce_min4(v); return min(v1,shuffle<1,0>(v1));}
INLINE avxf vreduce_max2(const avxf& v) {return max(v,shuffle<1,0,3,2>(v));}
INLINE avxf vreduce_max4(const avxf& v) {avxf v1 = vreduce_max2(v); return max(v1,shuffle<2,3,0,1>(v1));}
INLINE avxf vreduce_max (const avxf& v) {avxf v1 = vreduce_max4(v); return max(v1,shuffle<1,0>(v1));}
INLINE avxf vreduce_add2(const avxf& v) {return v + shuffle<1,0,3,2>(v);}
INLINE avxf vreduce_add4(const avxf& v) {avxf v1 = vreduce_add2(v); return v1 + shuffle<2,3,0,1>(v1);}
INLINE avxf vreduce_add (const avxf& v) {avxf v1 = vreduce_add4(v); return v1 + shuffle<1,0>(v1);}
INLINE float reduce_min(const avxf& v) {return _mm_cvtss_f32(extract<0>(vreduce_min(v)));}
INLINE float reduce_max(const avxf& v) {return _mm_cvtss_f32(extract<0>(vreduce_max(v)));}
INLINE float reduce_add(const avxf& v) {return _mm_cvtss_f32(extract<0>(vreduce_add(v)));}
INLINE size_t select_min(const avxf& v) {return __bsf(movemask(v == vreduce_min(v)));}
INLINE size_t select_max(const avxf& v) {return __bsf(movemask(v == vreduce_max(v)));}

// memory load and store operations
INLINE avxf load8f(const void* const a) {
  return _mm256_load_ps((const float*)a);
}

INLINE void store8f(void *ptr, const avxf& f) {
  return _mm256_store_ps((float*)ptr,f);
}

INLINE void store8f(const avxb& mask, void *ptr, const avxf& f) {
  return _mm256_maskstore_ps((float*)ptr,(__m256i)mask,f);
}

#if defined (__AVX2__)
INLINE avxf load8f_nt(void* ptr) {
  return _mm256_castsi256_ps(_mm256_stream_load_si256((__m256i*)ptr));
}
#endif

INLINE void store8f_nt(void* ptr, const avxf& v) {
  _mm256_stream_ps((float*)ptr,v);
}

INLINE avxf broadcast4f(const void* ptr) {
  return _mm256_broadcast_ps((__m128*)ptr);
}

// euclidian space operators
INLINE avxf dot (const avxf& a, const avxf& b) {return vreduce_add4(a*b);}
INLINE avxf cross (const avxf& a, const avxf& b) {
  const avxf a0 = a;
  const avxf b0 = shuffle<1,2,0,3>(b);
  const avxf a1 = shuffle<1,2,0,3>(a);
  const avxf b1 = b;
  return shuffle<1,2,0,3>(msub(a0,b0,a1*b1));
}
} /* namespace q */
#undef op

