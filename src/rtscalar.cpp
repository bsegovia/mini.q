/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rtscalar.cpp -> visibility ray and shadow ray generation
 -------------------------------------------------------------------------*/
#include "rt.hpp"
#include "bvh.hpp"

namespace q {
namespace rt {

void visibilitypacket(const camera &RESTRICT cam,
                      raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize)
{
  u32 idx = 0;
  for (u32 y = 0; y < u32(TILESIZE); ++y)
  for (u32 x = 0; x < u32(TILESIZE); ++x, ++idx) {
    const auto ray = cam.generate(screensize.x, screensize.y, tileorg.x+x, tileorg.y+y);
    p.setdir(ray.dir, idx);
  }
  p.raynum = TILESIZE*TILESIZE;
  p.sharedorg = cam.org;
  p.flags = raypacket::SHAREDORG;
}
void visibilitypackethit(packethit &hit) {
  loopi(MAXRAYNUM) {
    hit.id[i] = ~0x0u;
    hit.t[i] = FLT_MAX;
  }
}
} /* namespace rt */
} /* namespace q */
