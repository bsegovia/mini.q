/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhavx.hpp -> exposes bvh avx routines
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace rt {
namespace avx {
void closest(const struct intersector&, const struct raypacket&, struct packethit&);
void occluded(const struct intersector&, const struct raypacket&, struct packetshadow&);
} /* namespace avx */
} /* namespace rt */
} /* namespace q */
