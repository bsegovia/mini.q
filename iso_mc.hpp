/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - iso_mc.hpp -> exposes marching cube algorithm
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
// define a vertex as two indices in the unit cube
typedef vec2i mvert;

// we simply provide the the distance field value for each value
typedef float mcell[8];

// we return as most 16 triangles per cell
int tesselate(const mcell &cell, mvert *triangles);
} /* namespace q */

