/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ssei.hpp -> implements integer operations with SSE vectors
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
 - 4-wide SSE integer type
 -------------------------------------------------------------------------*/
struct ssei {
  typedef sseb Mask;  // mask type
  typedef ssei Int;   // int type
  typedef ssef Float; // float type
  enum   {size = 4};                  // number of SIMD elements
  union  {__m128i m128; s32 i[4];}; // data

  // constructors, assignment & cast ops
  INLINE ssei           () {}
  INLINE ssei           (const ssei& a) {m128 = a.m128;}
  INLINE ssei& op=(const ssei& a) {m128 = a.m128; return *this;}
  INLINE ssei(const __m128i a) : m128(a) {}
  INLINE op const __m128i&(void) const {return m128;}
  INLINE op       __m128i&(void)       {return m128;}
  INLINE explicit ssei (const s32* const a) : m128(_mm_loadu_si128((__m128i*)a)) {}
  INLINE ssei(const s32& a) : m128(_mm_shuffle_epi32(_mm_castps_si128(_mm_load_ss((float*)&a)), _MM_SHUFFLE(0, 0, 0, 0))) {}
  INLINE ssei(s32  a, s32  b) : m128(_mm_set_epi32(b, a, b, a)) {}
  INLINE ssei(s32  a, s32  b, s32  c, s32  d) : m128(_mm_set_epi32(d, c, b, a)) {}
  INLINE explicit ssei(const __m128 a) : m128(_mm_cvtps_epi32(a)) {}

  // constants
  INLINE ssei(zerotype) : m128(_mm_setzero_si128()) {}
  INLINE ssei(onetype) : m128(_mm_set_epi32(1, 1, 1, 1)) {}

  // array access
  INLINE const s32& op [](const size_t index) const {assert(index < 4); return i[index];}
  INLINE       s32& op [](const size_t index)       {assert(index < 4); return i[index];}
};

// unary ops
INLINE ssei cast(const __m128& a) {return _mm_castps_si128(a);}
INLINE ssei op +(const ssei& a) {return a;}
INLINE ssei op -(const ssei& a) {return _mm_sub_epi32(_mm_setzero_si128(), a.m128);}
#if defined(__SSSE3__)
INLINE ssei abs(const ssei& a) {return _mm_abs_epi32(a.m128);}
#endif

// binary ops
INLINE ssei op+ (const ssei& a, const ssei& b) {return _mm_add_epi32(a.m128, b.m128);}
INLINE ssei op+ (const ssei& a, const s32&  b) {return a + ssei(b);}
INLINE ssei op+ (const s32&  a, const ssei& b) {return ssei(a) + b;}
INLINE ssei op- (const ssei& a, const ssei& b) {return _mm_sub_epi32(a.m128, b.m128);}
INLINE ssei op- (const ssei& a, const s32&  b) {return a - ssei(b);}
INLINE ssei op- (const s32&  a, const ssei& b) {return ssei(a) - b;}

#if defined(__SSE4_1__)
INLINE ssei op* (const ssei& a, const ssei& b) {return _mm_mullo_epi32(a.m128, b.m128);}
INLINE ssei op* (const ssei& a, const s32&  b) {return a * ssei(b);}
INLINE ssei op* (const s32&  a, const ssei& b) {return ssei(a) * b;}
#endif

INLINE ssei op& (const ssei& a, const ssei& b) {return _mm_and_si128(a.m128, b.m128);}
INLINE ssei op& (const ssei& a, const s32&  b) {return a & ssei(b);}
INLINE ssei op& (const s32& a, const ssei& b) {return ssei(a) & b;}
INLINE ssei op| (const ssei& a, const ssei& b) {return _mm_or_si128(a.m128, b.m128);}
INLINE ssei op| (const ssei& a, const s32&  b) {return a | ssei(b);}
INLINE ssei op| (const s32& a, const ssei& b) {return ssei(a) | b;}
INLINE ssei op^ (const ssei& a, const ssei& b) {return _mm_xor_si128(a.m128, b.m128);}
INLINE ssei op^ (const ssei& a, const s32&  b) {return a ^ ssei(b);}
INLINE ssei op^ (const s32& a, const ssei& b) {return ssei(a) ^ b;}
INLINE ssei op<< (const ssei& a, const s32& n) {return _mm_slli_epi32(a.m128, n);}
INLINE ssei op>> (const ssei& a, const s32& n) {return _mm_srai_epi32(a.m128, n);}
INLINE ssei sra(const ssei& a, const s32& b) {return _mm_srai_epi32(a.m128, b);}
INLINE ssei srl(const ssei& a, const s32& b) {return _mm_srli_epi32(a.m128, b);}

#if defined(__SSE4_1__)
INLINE ssei min(const ssei& a, const ssei& b) {return _mm_min_epi32(a.m128, b.m128);}
INLINE ssei min(const ssei& a, const s32&  b) {return min(a,ssei(b));}
INLINE ssei min(const s32&  a, const ssei& b) {return min(ssei(a),b);}
INLINE ssei max(const ssei& a, const ssei& b) {return _mm_max_epi32(a.m128, b.m128);}
INLINE ssei max(const ssei& a, const s32&  b) {return max(a,ssei(b));}
INLINE ssei max(const s32&  a, const ssei& b) {return max(ssei(a),b);}
#endif

// assignment ops
INLINE ssei& op +=(ssei& a, const ssei& b) {return a = a + b;}
INLINE ssei& op +=(ssei& a, const s32&  b) {return a = a + b;}
INLINE ssei& op -=(ssei& a, const ssei& b) {return a = a - b;}
INLINE ssei& op -=(ssei& a, const s32&  b) {return a = a - b;}

#if defined(__SSE4_1__)
INLINE ssei& op *=(ssei& a, const ssei& b) {return a = a * b;}
INLINE ssei& op *=(ssei& a, const s32&  b) {return a = a * b;}
#endif

INLINE ssei& op &=(ssei& a, const ssei& b) {return a = a & b;}
INLINE ssei& op &=(ssei& a, const s32&  b) {return a = a & b;}
INLINE ssei& op |=(ssei& a, const ssei& b) {return a = a | b;}
INLINE ssei& op |=(ssei& a, const s32&  b) {return a = a | b;}
INLINE ssei& op <<=(ssei& a, const s32&  b) {return a = a << b;}
INLINE ssei& op >>=(ssei& a, const s32&  b) {return a = a >> b;}

// comparison ops + select
INLINE sseb op ==(const ssei& a, const ssei& b) {return _mm_castsi128_ps(_mm_cmpeq_epi32 (a.m128, b.m128));}
INLINE sseb op ==(const ssei& a, const s32& b) {return a == ssei(b);}
INLINE sseb op ==(const s32& a, const ssei& b) {return ssei(a) == b;}
INLINE sseb op !=(const ssei& a, const ssei& b) {return !(a == b);}
INLINE sseb op !=(const ssei& a, const s32& b) {return a != ssei(b);}
INLINE sseb op !=(const s32& a, const ssei& b) {return ssei(a) != b;}
INLINE sseb op < (const ssei& a, const ssei& b) {return _mm_castsi128_ps(_mm_cmplt_epi32 (a.m128, b.m128));}
INLINE sseb op < (const ssei& a, const s32& b) {return a <  ssei(b);}
INLINE sseb op < (const s32& a, const ssei& b) {return ssei(a) <  b;}
INLINE sseb op >=(const ssei& a, const ssei& b) {return !(a <  b);}
INLINE sseb op >=(const ssei& a, const s32& b) {return a >= ssei(b);}
INLINE sseb op >=(const s32& a, const ssei& b) {return ssei(a) >= b;}
INLINE sseb op > (const ssei& a, const ssei& b) {return _mm_castsi128_ps(_mm_cmpgt_epi32 (a.m128, b.m128));}
INLINE sseb op > (const ssei& a, const s32& b) {return a >  ssei(b);}
INLINE sseb op > (const s32& a, const ssei& b) {return ssei(a) >  b;}
INLINE sseb op <=(const ssei& a, const ssei& b) {return !(a >  b);}
INLINE sseb op <=(const ssei& a, const s32& b) {return a <= ssei(b);}
INLINE sseb op <=(const s32& a, const ssei& b) {return ssei(a) <= b;}

INLINE const ssei select(const sseb& m, const ssei& t, const ssei& f) {
#if defined(__SSE4_1__)
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(f), _mm_castsi128_ps(t), m));
#else
  return _mm_or_si128(_mm_and_si128(m, t), _mm_andnot_si128(m, f));
#endif
}

#if defined(__SSE4_1__) && !defined(__clang__)
INLINE const ssei select(const int m, const ssei& t, const ssei& f) {
  return _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(f), _mm_castsi128_ps(t), m));
}
#else
INLINE const ssei select(const int mask, const ssei& t, const ssei& f) {
  return select(sseb(mask),t,f);
}
#endif

// movement/shifting/shuffling functions
INLINE ssei unpacklo(const ssei& a, const ssei& b) {
  return _mm_castps_si128(_mm_unpacklo_ps(_mm_castsi128_ps(a.m128),
    _mm_castsi128_ps(b.m128)));
}
INLINE ssei unpackhi(const ssei& a, const ssei& b) {
  return _mm_castps_si128(_mm_unpackhi_ps(_mm_castsi128_ps(a.m128),
    _mm_castsi128_ps(b.m128)));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE ssei shuffle(const ssei& a) {
  return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE ssei shuffle(const ssei& a, const ssei& b) {
  return _mm_castps_si128(_mm_shuffle_ps(
    _mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
}

#if defined(__SSE3__)
template<> INLINE ssei shuffle<0,0,2,2>(const ssei& a) {
  return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a)));
}
template<> INLINE ssei shuffle<1,1,3,3>(const ssei& a) {
  return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a)));
}
template<> INLINE ssei shuffle<0,1,0,1>(const ssei& a) {
  return _mm_castpd_si128(_mm_movedup_pd (_mm_castsi128_pd(a)));
}
#endif

template<size_t i0> INLINE const ssei shuffle(const ssei& b) {
  return shuffle<i0,i0,i0,i0>(b);
}

#if defined(__SSE4_1__)
template<size_t src>
INLINE int extract(const ssei& b) {return _mm_extract_epi32(b, src);}
template<size_t dst>
INLINE const ssei insert(const ssei& a, const s32 b) {return _mm_insert_epi32(a, b, dst);}
#else
template<size_t src> INLINE
int extract(const ssei& b) {return b[src];}
template<size_t dst> INLINE
ssei insert(const ssei& a, const s32 b) {ssei c = a; c[dst] = b; return c;}
#endif

// reductions
#if defined(__SSE4_1__)
INLINE ssei vreduce_min(const ssei &v) {ssei h = min(shuffle<1,0,3,2>(v),v); return min(shuffle<2,3,0,1>(h),h);}
INLINE ssei vreduce_max(const ssei &v) {ssei h = max(shuffle<1,0,3,2>(v),v); return max(shuffle<2,3,0,1>(h),h);}
INLINE ssei vreduce_add(const ssei &v) {ssei h = shuffle<1,0,3,2>(v)   + v ; return shuffle<2,3,0,1>(h)   + h ;}
INLINE int reduce_min(const ssei &v) {return extract<0>(vreduce_min(v));}
INLINE int reduce_max(const ssei &v) {return extract<0>(vreduce_max(v));}
INLINE int reduce_add(const ssei &v) {return extract<0>(vreduce_add(v));}
INLINE size_t select_min(const ssei &v) {return __bsf(movemask(v == vreduce_min(v)));}
INLINE size_t select_max(const ssei &v) {return __bsf(movemask(v == vreduce_max(v)));}
#else
INLINE int reduce_min(const ssei& v) {return min(v[0],v[1],v[2],v[3]);}
INLINE int reduce_max(const ssei& v) {return max(v[0],v[1],v[2],v[3]);}
INLINE int reduce_add(const ssei& v) {return v[0]+v[1]+v[2]+v[3];}
#endif

// memory load and store operations
INLINE ssei load4i(const void* const a) {return _mm_load_si128((__m128i*)a);}
INLINE void store4i(void* ptr, const ssei& v) {_mm_store_si128((__m128i*)ptr,v);}
INLINE void storeu4i(void* ptr, const ssei& v) {_mm_storeu_si128((__m128i*)ptr,v);}

INLINE void store4i(const sseb& mask, void* ptr, const ssei& i) {
#if defined (__AVX__)
  _mm_maskstore_ps((float*)ptr,(__m128i)mask,_mm_castsi128_ps(i));
#else
  *(ssei*)ptr = select(mask,i,*(ssei*)ptr);
#endif
}

INLINE ssei load4i_nt (void* ptr) {
#if defined(__SSE4_1__)
  return _mm_stream_load_si128((__m128i*)ptr);
#else
  return _mm_load_si128((__m128i*)ptr);
#endif
}

INLINE void store4i_nt(void* ptr, const ssei& v) {
#if defined(__SSE4_1__)
  _mm_stream_ps((float*)ptr,_mm_castsi128_ps(v));
#else
  _mm_store_si128((__m128i*)ptr,v);
#endif
}
} /* namespace q */
#undef op

