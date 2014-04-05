/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhsse.hpp -> exposes bvh sse routines
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace rt {
namespace sse {
void closest(const struct intersector&, const struct raypacket&, struct packethit&);
void occluded(const struct intersector&, const struct raypacket&, struct packetshadow&);
} /* namespace sse */
} /* namespace rt */
} /* namespace q */
