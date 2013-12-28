/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - iso_mc.hpp -> exposes marching cube algorithm
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"
#include "iso.hpp"

namespace q {
namespace iso {

// tesselate along a grid the given distance field function
mesh mc(const grid &grid, distance_field f);

} /* namespace iso */
} /* namespace q */

