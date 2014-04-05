/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhsse.hpp -> exposes bvh sse routines
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"

namespace q {
namespace rt {
namespace sse {
// ray tracing routines (visiblity and shadow rays)
void closest(const struct intersector&, const struct raypacket&, struct packethit&);
void occluded(const struct intersector&, const struct raypacket&, struct packetshadow&);

// ray packet generation
void visibilitypacket(const struct camera &RESTRICT cam,
                      struct raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize);
void visibilitypackethit(packethit &hit);
} /* namespace sse */
} /* namespace rt */
} /* namespace q */
