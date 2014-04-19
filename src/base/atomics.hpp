/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - atomics.hpp -> exposes atomic functions and classes
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "utility.hpp"

namespace q {
struct atomic : noncopyable {
public:
  INLINE atomic(void) {}
  INLINE atomic(s32 data) : data(data) {}
  INLINE atomic& operator =(const s32 input) { data = input; return *this; }
  INLINE operator s32(void) const { return data; }
  INLINE friend void storerelease(atomic &value, s32 x) { storerelease(&value.data, x); }
  INLINE friend s32 operator+=   (atomic &value, s32 input) { return atomic_add(&value.data, input) + input; }
  INLINE friend s32 operator++   (atomic &value) { return atomic_add(&value.data,  1) + 1; }
  INLINE friend s32 operator--   (atomic &value) { return atomic_add(&value.data, -1) - 1; }
  INLINE friend s32 operator++   (atomic &value, s32) { return atomic_add(&value.data,  1); }
  INLINE friend s32 operator--   (atomic &value, s32) { return atomic_add(&value.data, -1); }
  INLINE friend s32 cmpxchg      (atomic &value, s32 v, s32 c) { return atomic_cmpxchg(&value.data,v,c); }
private:
  volatile s32 data;
};
} /* namespace q */

