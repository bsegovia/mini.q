/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - qef.hpp -> exposes quadratic error function
 - original code by Ronen Tzur (rtzur@shani.net)
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace qef {
  // QEF, a class implementing the quadric error function
  //      E[x] = P - Ni . Pi
  // Given at least three points Pi, each with its respective normal vector Ni,
  // that describe at least two planes, the QEF evalulates to the point x.
  vec3f evaluate(double mat[][3], double *vec, int rows);
} /* namespace qef */
} /* namespace q */

