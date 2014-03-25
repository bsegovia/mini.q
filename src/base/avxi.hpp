/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - avxi.hpp -> implements integer operations with AVX vectors
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
 - 8-wide AVX int type
 -------------------------------------------------------------------------*/
struct avxi {
  typedef avxb masktype; // mask type for us
  enum   {size = 8};     // number of SIMD elements
  union  {               // data
    __m256i m256;
    s32 v[8];
 };

  // constructors, assignment & cast ops
  INLINE avxi() {}
  INLINE avxi(const avxi& a) {m256 = a.m256;}
  INLINE avxi& op=(const avxi& a) {m256 = a.m256; return *this;}
  INLINE avxi(const __m256i a) : m256(a) {}
  INLINE op const __m256i&(void) const {return m256;}
  INLINE op       __m256i&(void)       {return m256;}
  INLINE explicit avxi(const ssei& a) :
    m256(_mm256_insertf128_si256(_mm256_castsi128_si256(a),a,1)) {}
  INLINE avxi(const ssei& a, const ssei& b) :
    m256(_mm256_insertf128_si256(_mm256_castsi128_si256(a),b,1)) {}
  INLINE avxi(const __m128i& a, const __m128i& b) :
    m256(_mm256_insertf128_si256(_mm256_castsi128_si256(a),b,1)) {}
  INLINE explicit avxi (const s32* const a) :
    m256(_mm256_castps_si256(_mm256_loadu_ps((const float*)a))) {}
  INLINE avxi(s32 a) : m256(_mm256_set1_epi32(a)) {}
  INLINE avxi(s32 a, s32 b) : m256(_mm256_set_epi32(b, a, b, a, b, a, b, a)) {}
  INLINE avxi(s32 a, s32 b, s32 c, s32 d) :
    m256(_mm256_set_epi32(d, c, b, a, d, c, b, a)) {}
  INLINE avxi(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h) :
    m256(_mm256_set_epi32(h, g, f, e, d, c, b, a)) {}
  INLINE explicit avxi(const __m256 a) : m256(_mm256_cvtps_epi32(a)) {}

  // constants
  INLINE avxi(zerotype) : m256(_mm256_setzero_si256()) {}
  INLINE avxi(onetype)  : m256(_mm256_set1_epi32(1)) {}

  // array access
  INLINE const s32& op [](const size_t i) const {assert(i < 8); return v[i];}
  INLINE       s32& op [](const size_t i)       {assert(i < 8); return v[i];}
};

// unary ops
INLINE avxi op+ (const avxi& a) {return a;}
INLINE avxi op- (const avxi& a) {return _mm256_sub_epi32(_mm256_setzero_si256(), a.m256);}
INLINE avxi abs(const avxi& a) {return _mm256_abs_epi32(a.m256);}
INLINE avxi cast(const __m256& a) {return _mm256_castps_si256(a);}

// Binary Operators
INLINE avxi op+ (const avxi& a, const avxi& b) {return _mm256_add_epi32(a.m256, b.m256);}
INLINE avxi op+ (const avxi& a, const s32 b) {return a + avxi(b);}
INLINE avxi op+ (const s32 a, const avxi& b) {return avxi(a) + b;}
INLINE avxi op- (const avxi& a, const avxi& b) {return _mm256_sub_epi32(a.m256, b.m256);}
INLINE avxi op- (const avxi& a, const s32 b) {return a - avxi(b);}
INLINE avxi op- (const s32 a, const avxi& b) {return avxi(a) - b;}
INLINE avxi op* (const avxi& a, const avxi& b) {return _mm256_mullo_epi32(a.m256, b.m256);}
INLINE avxi op* (const avxi& a, const s32 b) {return a * avxi(b);}
INLINE avxi op* (const s32 a, const avxi& b) {return avxi(a) * b;}
INLINE avxi op& (const avxi& a, const avxi& b) {return _mm256_and_si256(a.m256, b.m256);}
INLINE avxi op& (const avxi& a, const s32 b) {return a & avxi(b);}
INLINE avxi op& (const s32 a, const avxi& b) {return avxi(a) & b;}
INLINE avxi op| (const avxi& a, const avxi& b) {return _mm256_or_si256(a.m256, b.m256);}
INLINE avxi op| (const avxi& a, const s32 b) {return a | avxi(b);}
INLINE avxi op| (const s32 a, const avxi& b) {return avxi(a) | b;}
INLINE avxi op^ (const avxi& a, const avxi& b) {return _mm256_xor_si256(a.m256, b.m256);}
INLINE avxi op^ (const avxi& a, const s32 b) {return a ^ avxi(b);}
INLINE avxi op^ (const s32 a, const avxi& b) {return avxi(a) ^ b;}
INLINE avxi op<< (const avxi& a, const s32 n) {return _mm256_slli_epi32(a.m256, n);}
INLINE avxi op>> (const avxi& a, const s32 n) {return _mm256_srai_epi32(a.m256, n);}
INLINE avxi sra (const avxi& a, const s32 b) {return _mm256_srai_epi32(a.m256, b);}
INLINE avxi srl (const avxi& a, const s32 b) {return _mm256_srli_epi32(a.m256, b);}
INLINE avxi min(const avxi& a, const avxi& b) {return _mm256_min_epi32(a.m256, b.m256);}
INLINE avxi min(const avxi& a, const s32 b) {return min(a,avxi(b));}
INLINE avxi min(const s32 a, const avxi& b) {return min(avxi(a),b);}
INLINE avxi max(const avxi& a, const avxi& b) {return _mm256_max_epi32(a.m256, b.m256);}
INLINE avxi max(const avxi& a, const s32 b) {return max(a,avxi(b));}
INLINE avxi max(const s32 a, const avxi& b) {return max(avxi(a),b);}

// assignment operators
INLINE avxi& op+= (avxi& a, const avxi& b) {return a = a + b;}
INLINE avxi& op+= (avxi& a, const s32  b) {return a = a + b;}
INLINE avxi& op-= (avxi& a, const avxi& b) {return a = a - b;}
INLINE avxi& op-= (avxi& a, const s32  b) {return a = a - b;}
INLINE avxi& op*= (avxi& a, const avxi& b) {return a = a * b;}
INLINE avxi& op*= (avxi& a, const s32  b) {return a = a * b;}
INLINE avxi& op&= (avxi& a, const avxi& b) {return a = a & b;}
INLINE avxi& op&= (avxi& a, const s32  b) {return a = a & b;}
INLINE avxi& op|= (avxi& a, const avxi& b) {return a = a | b;}
INLINE avxi& op|= (avxi& a, const s32  b) {return a = a | b;}
INLINE avxi& op<<= (avxi& a, const s32  b) {return a = a << b;}
INLINE avxi& op>>= (avxi& a, const s32  b) {return a = a >> b;}

// comparison operators + select
INLINE avxb op ==(const avxi& a, const avxi& b) {
  return _mm256_castsi256_ps(_mm256_cmpeq_epi32 (a.m256, b.m256));
}
INLINE avxb op < (const avxi& a, const avxi& b) {
  return _mm256_castsi256_ps(_mm256_cmpgt_epi32 (b.m256, a.m256));
}
INLINE avxb op > (const avxi& a, const avxi& b) {
  return _mm256_castsi256_ps(_mm256_cmpgt_epi32 (a.m256, b.m256));
}
INLINE avxb op== (const avxi& a, const s32 b) {return a == avxi(b);}
INLINE avxb op== (const s32 a, const avxi& b) {return avxi(a) == b;}
INLINE avxb op!= (const avxi& a, const avxi& b) {return !(a == b);}
INLINE avxb op!= (const avxi& a, const s32 b) {return a != avxi(b);}
INLINE avxb op!= (const s32 a, const avxi& b) {return avxi(a) != b;}
INLINE avxb op<  (const avxi& a, const s32 b) {return a <  avxi(b);}
INLINE avxb op<  (const s32 a, const avxi& b) {return avxi(a) <  b;}
INLINE avxb op>= (const avxi& a, const avxi& b) {return !(a <  b);}
INLINE avxb op>= (const avxi& a, const s32 b) {return a >= avxi(b);}
INLINE avxb op>= (const s32 a, const avxi& b) {return avxi(a) >= b;}
INLINE avxb op>  (const avxi& a, const s32 b) {return a >  avxi(b);}
INLINE avxb op>  (const s32 a, const avxi& b) {return avxi(a) >  b;}
INLINE avxb op<= (const avxi& a, const avxi& b) {return !(a >  b);}
INLINE avxb op<= (const avxi& a, const s32 b) {return a <= avxi(b);}
INLINE avxb op<= (const s32 a, const avxi& b) {return avxi(a) <= b;}
INLINE avxi select(const avxb& m, const avxi& t, const avxi& f) {
  return _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(f), _mm256_castsi256_ps(t), m)); 
}

#if !defined(__clang__)
INLINE const avxi select(const int m, const avxi& t, const avxi& f) {
  return _mm256_blend_epi32(f,t,m);
}
#else
INLINE const avxi select(const int m, const avxi& t, const avxi& f) {
  return select(avxb(m),t,f);
}
#endif

// movement/shifting/shuffling functions
INLINE avxi unpacklo(const avxi& a, const avxi& b) {return _mm256_unpacklo_epi32(a.m256, b.m256);}
INLINE avxi unpackhi(const avxi& a, const avxi& b) {return _mm256_unpackhi_epi32(a.m256, b.m256);}

template<size_t i>
INLINE avxi shuffle(const avxi& a) {
  return _mm256_castps_si256(_mm256_permute_ps(_mm256_castsi256_ps(a), _MM_SHUFFLE(i, i, i, i)));
}

template<size_t i0, size_t i1>
INLINE avxi shuffle(const avxi& a) {
  return _mm256_permute2f128_si256(a, a, (i1 << 4) | (i0 << 0));
}

template<size_t i0, size_t i1>
INLINE const avxi shuffle(const avxi& a,  const avxi& b) {
  return _mm256_permute2f128_si256(a, b, (i1 << 4) | (i0 << 0));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE const avxi shuffle(const avxi& a) {
  return _mm256_castps_si256(_mm256_permute_ps(
          _mm256_castsi256_ps(a), _MM_SHUFFLE(i3, i2, i1, i0)));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE const avxi shuffle(const avxi& a, const avxi& b) {
  return _mm256_castps_si256(_mm256_shuffle_ps(
          _mm256_castsi256_ps(a), _mm256_castsi256_ps(b),
           _MM_SHUFFLE(i3, i2, i1, i0)));
}

template<>
INLINE avxi shuffle<0, 0, 2, 2>(const avxi& b) {
  return _mm256_castps_si256(_mm256_moveldup_ps(_mm256_castsi256_ps(b)));
}
template<>
INLINE avxi shuffle<1, 1, 3, 3>(const avxi& b) {
  return _mm256_castps_si256(_mm256_movehdup_ps(_mm256_castsi256_ps(b)));
}
template<>
INLINE avxi shuffle<0, 1, 0, 1>(const avxi& b) {
  return _mm256_castps_si256(_mm256_castpd_ps(
          _mm256_movedup_pd(_mm256_castps_pd(_mm256_castsi256_ps(b)))));
}

INLINE avxi broadcast(const int* ptr) {
  return _mm256_castps_si256(_mm256_broadcast_ss((const float*)ptr));
}
template<size_t i>
INLINE avxi insert (const avxi& a, const ssei& b) {
  return _mm256_insertf128_si256 (a,b,i);
}
template<size_t i>
INLINE ssei extract(const avxi& a) {
  return _mm256_extractf128_si256(a, i);
}
INLINE avxi permute(const avxi& a, const __m256i& index) {
  return _mm256_permutevar8x32_epi32(a,index);
}

/// reductions
INLINE avxi vreduce_min2(const avxi& v) {return min(v,shuffle<1,0,3,2>(v));}
INLINE avxi vreduce_min4(const avxi& v) {avxi v1 = vreduce_min2(v); return min(v1,shuffle<2,3,0,1>(v1));}
INLINE avxi vreduce_min (const avxi& v) {avxi v1 = vreduce_min4(v); return min(v1,shuffle<1,0>(v1));}
INLINE avxi vreduce_max2(const avxi& v) {return max(v,shuffle<1,0,3,2>(v));}
INLINE avxi vreduce_max4(const avxi& v) {avxi v1 = vreduce_max2(v); return max(v1,shuffle<2,3,0,1>(v1));}
INLINE avxi vreduce_max (const avxi& v) {avxi v1 = vreduce_max4(v); return max(v1,shuffle<1,0>(v1));}
INLINE avxi vreduce_add2(const avxi& v) {return v + shuffle<1,0,3,2>(v);}
INLINE avxi vreduce_add4(const avxi& v) {avxi v1 = vreduce_add2(v); return v1 + shuffle<2,3,0,1>(v1);}
INLINE avxi vreduce_add (const avxi& v) {avxi v1 = vreduce_add4(v); return v1 + shuffle<1,0>(v1);}
INLINE int reduce_min(const avxi& v) {return extract<0>(extract<0>(vreduce_min(v)));}
INLINE int reduce_max(const avxi& v) {return extract<0>(extract<0>(vreduce_max(v)));}
INLINE int reduce_add(const avxi& v) {return extract<0>(extract<0>(vreduce_add(v)));}
INLINE size_t select_min(const avxi& v) {return __bsf(movemask(v == vreduce_min(v)));}
INLINE size_t select_max(const avxi& v) {return __bsf(movemask(v == vreduce_max(v)));}

// memory load and store operations
INLINE const avxi load8i(const int* const i) {
  return _mm256_load_si256((__m256i*)i); 
}
INLINE const avxi uload8i(const int* const i) {
  return _mm256_loadu_si256((__m256i*)i); 
}
INLINE void store8i(void *ptr, const avxi& i) {
  _mm256_store_si256((__m256i*)ptr,i);
}
INLINE void store8i(const avxb &mask, void *ptr, const avxi& i) {
  _mm256_maskstore_epi32((int*)ptr,mask,i);
}

#if defined (__AVX2__)
INLINE avxi load8i_nt(void* ptr) {
  return _mm256_stream_load_si256((__m256i*)ptr);
}
#endif

INLINE void store8i_nt(void* ptr, const avxi& v) {
  _mm256_stream_ps((float*)ptr,_mm256_castsi256_ps(v));
}
} /* namespace q */
#undef op

