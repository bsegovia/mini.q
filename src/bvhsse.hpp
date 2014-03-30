/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhsse.hpp -> exposes bvh sse routines
 -------------------------------------------------------------------------*/
namespace q {
namespace bvh {
namespace sse {
void closest(const intersector &tree, const raypacket &p, packethit &hit);
void occluded(const intersector &tree, const raypacket &p, packetshadow &s);
} /* namespace sse */
} /* namespace bvh */
} /* namespace q */
