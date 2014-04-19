/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.cpp -> implements standard library routines
 -------------------------------------------------------------------------*/
#include "stl.hpp"

namespace q {

u32 murmurhash2(const void *key, int len, u32 seed) {
  const u32 m = 0x5bd1e995;
  const int r = 24;
  u32 h = seed ^ len;
  const u8* data = (const unsigned char *)key;
  while(len >= 4) {
    u32 k = *(u32 *)data;
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    len -= 4;
  }
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

} /* namespace q */

