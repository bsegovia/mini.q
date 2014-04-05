/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhavx.hpp -> exposes bvh avx routines
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace rt {
namespace avx {
// ray tracing routines (visiblity and shadow rays)
void closest(const struct intersector&, const struct raypacket&, struct packethit&);
void occluded(const struct intersector&, const struct raypacket&, struct packetshadow&);

// ray packet generation
void visibilitypacket(const struct camera &RESTRICT cam,
                      struct raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize);
void visibilitypackethit(packethit &hit);
} /* namespace avx */
} /* namespace rt */
} /* namespace q */
