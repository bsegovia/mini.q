/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - bvhavx.hpp -> exposes bvh avx routines
 -------------------------------------------------------------------------*/
namespace q {
namespace bvh {
namespace avx {
void closest(const intersector &tree, const raypacket &p, packethit &hit);
void occluded(const intersector &tree, const raypacket &p, packetshadow &s);
} /* namespace avx */
} /* namespace bvh */
} /* namespace q */
