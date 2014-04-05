/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rtscalar.cpp -> visibility ray and shadow ray generation
 -------------------------------------------------------------------------*/
#pragma once
#include "rt.hpp"
#include "bvh.hpp"
namespace q {
namespace rt {

// ray tracing routines (visiblity and shadow rays)
void closest(const intersector&, const struct ray&, hit&);
bool occluded(const intersector&, const struct ray&);
void closest(const intersector&, const raypacket&, packethit&);
void occluded(const intersector&, const raypacket&, packetshadow&);

// ray packet generation
void visibilitypacket(const camera &RESTRICT cam,
                      raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize);
void visibilitypackethit(packethit &hit);
} /* namespace rt */
} /* namespace q */

