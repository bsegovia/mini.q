/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - hash.hpp -> implements default hash function
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include <cstring>

namespace q {
typedef u32 hash_value_t;

// fast 32 bits murmur hash and its generic version
u32 murmurhash2(const void *key, int len, u32 seed = 0xffffffffu);
template <typename T> INLINE u32 murmurhash2(const T &x) {
  return murmurhash2(&x, sizeof(T));
}

// default implementation of hasher using murmurhash2
template<typename T>
struct hash {
  hash_value_t operator()(const T &t) const { return murmurhash2(t); }
};
template<>
struct hash<const char*> {
  hash_value_t operator()(const char *str) const {
    return murmurhash2(str, strlen(str));
  }
};
} /* namespace q */

