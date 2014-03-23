/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sseb.hpp -> implements booleans operations with SSE vectors
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

namespace q {

/*-------------------------------------------------------------------------
 - 4-wide SSE bool type
 -------------------------------------------------------------------------*/
struct sseb {
  typedef struct sseb Mask;  // mask type
  typedef struct ssei Int;   // int type
  typedef struct ssef Float; // float type

  enum   {size = 4};                  // number of SIMD elements
  union  {__m128 m128; s32 v[4];};  // data

  // constructors, assignment & cast operators
  INLINE sseb() {}
  INLINE sseb(const sseb& other) {m128 = other.m128;}
  INLINE sseb &operator=(const sseb& other) {m128 = other.m128; return *this;}

  INLINE sseb(const __m128  input) : m128(input) {}
  INLINE operator const __m128&(void) const {return m128;}
  INLINE operator const __m128i(void) const {return _mm_castps_si128(m128);}
  INLINE operator const __m128d(void) const {return _mm_castps_pd(m128);}

  INLINE sseb(bool a)
    : m128(_mm_lookupmask_ps[(size_t(a)<<3) | (size_t(a)<<2) | (size_t(a)<<1) | size_t(a)]) {}
  INLINE sseb(bool a, bool b)
    : m128(_mm_lookupmask_ps[(size_t(b)<<3) | (size_t(a)<<2) | (size_t(b)<<1) | size_t(a)]) {}
  INLINE sseb(bool a, bool b, bool c, bool d)
    : m128(_mm_lookupmask_ps[(size_t(d)<<3) | (size_t(c)<<2) | (size_t(b)<<1) | size_t(a)]) {}
  INLINE sseb(int mask) {
    assert(mask >= 0 && mask < 16);
    m128 = _mm_lookupmask_ps[mask];
  }

  // constants
  INLINE sseb(falsetype) : m128(_mm_setzero_ps()) {}
  INLINE sseb(truetype) :m128(_mm_castsi128_ps(
    _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()))) {}

  // array access
  INLINE bool   operator[] (const size_t i) const {
    assert(i < 4); return (_mm_movemask_ps(m128) >> i) & 1;
  }
  INLINE s32& operator[] (const size_t i) {
    assert(i < 4); return v[i];
  }
};

// unary operators
INLINE sseb operator! (const sseb& a) {return _mm_xor_ps(a, sseb(truev));}

// binary operators
INLINE sseb operator& (const sseb& a, const sseb& b) {return _mm_and_ps(a, b);}
INLINE sseb operator| (const sseb& a, const sseb& b) {return _mm_or_ps (a, b);}
INLINE sseb operator^ (const sseb& a, const sseb& b) {return _mm_xor_ps(a, b);}

// assignment operators
INLINE sseb operator&= (sseb& a, const sseb& b) {return a = a & b;}
INLINE sseb operator|= (sseb& a, const sseb& b) {return a = a | b;}
INLINE sseb operator^= (sseb& a, const sseb& b) {return a = a ^ b;}

// comparison operators + select
INLINE sseb operator!= (const sseb& a, const sseb& b) {return _mm_xor_ps(a, b);}
INLINE sseb operator== (const sseb& a, const sseb& b) {return _mm_castsi128_ps(_mm_cmpeq_epi32(a, b));}

INLINE sseb select(const sseb& m, const sseb& t, const sseb& f) {
#if defined(__SSE4_1__)
  return _mm_blendv_ps(f, t, m);
#else
  return _mm_or_ps(_mm_and_ps(m, t), _mm_andnot_ps(m, f));
#endif
}

// movement/shifting/shuffling functions
INLINE sseb unpacklo(const sseb& a, const sseb& b) {return _mm_unpacklo_ps(a, b);}
INLINE sseb unpackhi(const sseb& a, const sseb& b) {return _mm_unpackhi_ps(a, b);}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE sseb shuffle(const sseb& a) {
  return _mm_shuffle_epi32(a, _MM_SHUFFLE(i3, i2, i1, i0));
}

template<size_t i0, size_t i1, size_t i2, size_t i3> INLINE
sseb shuffle(const sseb& a, const sseb& b) {
  return _mm_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

#if defined(__SSE3__)
template<> INLINE sseb shuffle<0, 0, 2, 2>(const sseb& a) {
  return _mm_moveldup_ps(a);
}
template<> INLINE sseb shuffle<1, 1, 3, 3>(const sseb& a) {
  return _mm_movehdup_ps(a);
}
template<> INLINE sseb shuffle<0, 1, 0, 1>(const sseb& a) {
  return _mm_castpd_ps(_mm_movedup_pd (a));
}
#endif

#if defined(__SSE4_1__)
template<size_t dst, size_t src, size_t clr>
INLINE sseb insert(const sseb& a, const sseb& b) {
  return _mm_insert_ps(a, b, (dst << 4) | (src << 6) | clr);
}
template<size_t dst, size_t src>
INLINE sseb insert(const sseb& a, const sseb& b) {
  return insert<dst, src, 0>(a, b);
}
template<size_t dst>
INLINE sseb insert(const sseb& a, const bool b) {
  return insert<dst,0>(a, sseb(b));
}
#endif

// reduction operations
#if defined(__SSE4_1__)
INLINE size_t popcnt(const sseb &a) {return __popcnt(_mm_movemask_ps(a));}
#else
INLINE size_t popcnt(const sseb &a) {return bool(a[0])+bool(a[1])+bool(a[2])+bool(a[3]);}
#endif

INLINE bool all (const sseb& b) {return _mm_movemask_ps(b) == 0xf;}
INLINE bool any (const sseb& b) {return _mm_movemask_ps(b) != 0x0;}
INLINE bool none(const sseb& b) {return _mm_movemask_ps(b) == 0x0;}
INLINE bool reduce_and(const sseb& a) {return _mm_movemask_ps(a) == 0xf;}
INLINE bool reduce_or (const sseb& a) {return _mm_movemask_ps(a) != 0x0;}
INLINE size_t movemask(const sseb& a) {return _mm_movemask_ps(a);}
} /* namespace q */

