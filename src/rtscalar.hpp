/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rtscalar.cpp -> visibility ray and shadow ray generation
 -------------------------------------------------------------------------*/
#pragma once
#include "rt.hpp"
#include "bvh.hpp"
namespace q {
namespace rt {
void visibilitypacket(const camera &RESTRICT cam,
                      raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize);
void visibilitypackethit(packethit &hit);
} /* namespace rt */
} /* namespace q */

