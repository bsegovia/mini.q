/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - avxi_emu.hpp -> implements integer operations for AVX with SSE vectors
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
 - 8-wide AVX bool type
 -------------------------------------------------------------------------*/
struct avxb {
  typedef avxb masktype; // mask type for us
  enum {size = 8};       // number of SIMD elements
  union {                // data
    __m256 m256;
    struct {__m128 l,h;};
    s32 v[8];
  };

  // constructors, assignment & cast operators
  INLINE avxb() {}
  INLINE avxb(const avxb& a) {m256 = a.m256;}
  INLINE avxb& op=(const avxb& a) {m256 = a.m256; return *this;}
  INLINE avxb(const __m256 a) : m256(a) {}
  INLINE op const __m256&(void) const {return m256;}
  INLINE op const __m256i(void) const {return _mm256_castps_si256(m256);}
  INLINE op const __m256d(void) const {return _mm256_castps_pd(m256);}

  INLINE avxb (const int a) {
#if defined (__AVX2__)
    const __m256i mask = _mm256_set_epi32(0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1);
    const __m256i b = _mm256_set1_epi32(a);
    const __m256i c = _mm256_and_si256(b,mask);
    m256 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(c,mask));
#else
    assert(a >= 0 && a < 32);
    l = _mm_lookupmask_ps[a & 0xF];
    h = _mm_lookupmask_ps[a >> 4];
#endif
 }

  INLINE avxb (const sseb& a) :
    m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),a,1)) {}
  INLINE avxb (const sseb& a, const  sseb& b) :
    m256(_mm256_insertf128_ps(_mm256_castps128_ps256(a),b,1)) {}
  INLINE avxb (const __m128 a, const __m128 b) : l(a), h(b) {}
  INLINE avxb (bool a) : m256(avxb(sseb(a), sseb(a))) {}
  INLINE avxb (bool a, bool b) : m256(avxb(sseb(a), sseb(b))) {}
  INLINE avxb (bool a, bool b, bool c, bool d) :
    m256(avxb(sseb(a,b), sseb(c,d))) {}
  INLINE avxb (bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h) :
    m256(avxb(sseb(a,b,c,d), sseb(e,f,g,h))) {}

  // constants
  INLINE avxb(falsetype) :
    m256(_mm256_setzero_ps()) {}
  INLINE avxb(truetype) :
    m256(_mm256_cmp_ps(_mm256_setzero_ps(), _mm256_setzero_ps(), _CMP_EQ_OQ)) {}

  // loads and stores
  static INLINE avxb load(const void* const ptr) {return *(__m256*)ptr;}

  // array access
  INLINE bool op [](const size_t i) const {
    assert(i < 8);
    return (_mm256_movemask_ps(m256) >> i) & 1;
  }
  INLINE s32& op [](const size_t i) {
    assert(i < 8);
    return v[i];
  }
};

// unary operators
INLINE avxb op !(const avxb &a) {return _mm256_xor_ps(a, avxb(truev));}

// binary operators
INLINE avxb op& (const avxb& a, const avxb& b) {return _mm256_and_ps(a, b);}
INLINE avxb op| (const avxb& a, const avxb& b) {return _mm256_or_ps (a, b);}
INLINE avxb op^ (const avxb& a, const avxb& b) {return _mm256_xor_ps(a, b);}
INLINE avxb op&= (avxb& a, const avxb& b) {return a = a & b;}
INLINE avxb op|= (avxb& a, const avxb& b) {return a = a | b;}
INLINE avxb op^= (avxb& a, const avxb& b) {return a = a ^ b;}

// comparison operators + select
INLINE avxb op !=(const avxb& a, const avxb& b) {
  return _mm256_xor_ps(a, b);
}
INLINE avxb op ==(const avxb& a, const avxb& b) {
  return _mm256_xor_ps(_mm256_xor_ps(a,b),avxb(truev));
}
INLINE avxb select(const avxb& mask, const avxb& t, const avxb& f) {
  return _mm256_blendv_ps(f, t, mask);
}

// Movement/Shifting/Shuffling Functions
INLINE avxb unpacklo(const avxb& a, const avxb& b) {return _mm256_unpacklo_ps(a.m256, b.m256); }
INLINE avxb unpackhi(const avxb& a, const avxb& b) {return _mm256_unpackhi_ps(a.m256, b.m256); }
INLINE avxb andnot(const avxb& a, const avxb& b) {return _mm256_andnot_ps(a.m256, b.m256);}

template<size_t i>
INLINE avxb shuffle(const avxb& a) {
  return _mm256_permute_ps(a, _MM_SHUFFLE(i, i, i, i));
}

template<size_t i0, size_t i1>
INLINE avxb shuffle(const avxb& a) {
  return _mm256_permute2f128_ps(a, a, (i1 << 4) | (i0 << 0));
}

template<size_t i0, size_t i1>
INLINE avxb shuffle(const avxb& a,  const avxb& b) {
  return _mm256_permute2f128_ps(a, b, (i1 << 4) | (i0 << 0));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE avxb shuffle(const avxb& a) {
  return _mm256_permute_ps(a, _MM_SHUFFLE(i3, i2, i1, i0));
}

template<size_t i0, size_t i1, size_t i2, size_t i3>
INLINE avxb shuffle(const avxb& a, const avxb& b) {
  return _mm256_shuffle_ps(a, b, _MM_SHUFFLE(i3, i2, i1, i0));
}

template<> INLINE avxb shuffle<0,0,2,2>(const avxb& b) {
  return _mm256_moveldup_ps(b);
}
template<> INLINE avxb shuffle<1,1,3,3>(const avxb& b) {
  return _mm256_movehdup_ps(b);
}
template<> INLINE avxb shuffle<0,1,0,1>(const avxb& b) {
  return _mm256_castpd_ps(_mm256_movedup_pd(_mm256_castps_pd(b)));
}

// memory load and store operations
INLINE avxb load8b(const void* const a) {
  return _mm256_load_ps((const float*)a);
}
INLINE void store8b(void *ptr, const avxb& b) {
  return _mm256_store_ps((float*)ptr,b);
}
INLINE void store8b(const avxb& mask, void *ptr, const avxb& b) {
  return _mm256_maskstore_ps((float*)ptr,(__m256i)mask,b);
}

template<size_t i>
INLINE avxb insert (const avxb& a, const sseb& b) {
  return _mm256_insertf128_ps (a,b,i);
}
template<size_t i>
INLINE sseb extract(const avxb& a) {
  return _mm256_extractf128_ps(a,i);
}

// reduction operations
INLINE size_t popcnt(const avxb& a)   {return __popcnt(_mm256_movemask_ps(a));}
INLINE bool reduce_and(const avxb& a) {return _mm256_movemask_ps(a) == (u32)0xff;}
INLINE bool reduce_or (const avxb& a) {return !_mm256_testz_ps(a,a);}
INLINE bool all(const avxb& a)  {return _mm256_movemask_ps(a) == (u32)0xff;}
INLINE bool none(const avxb& a) {return _mm256_testz_ps(a,a) != 0;}
INLINE bool any(const avxb& a)  {return !_mm256_testz_ps(a,a);}
INLINE u32 movemask(const avxb& a) {return _mm256_movemask_ps(a);}
} /* namespace q */
#undef op

