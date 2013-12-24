/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - marching_cube.hpp -> exposes marching cube algorithm
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
struct triangle {
  vec3f p[3];
};

struct gridcell {
  vec3f p[8];
  float val[8];
};

int tesselate(const gridcell &grid, triangle *triangles);
} /* namespace q */

