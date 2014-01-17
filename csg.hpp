/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace csg {
struct node *makescene();
void destroyscene(struct node *n);
float dist(const vec3f &pos, struct node *n);
} /* namespace csg */
} /* namespace q */

