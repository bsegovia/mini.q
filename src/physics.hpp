/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.hpp -> exposes physics routines
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"

namespace q {
namespace physics {

void moveplayer(game::dynent *pl, int moveres, bool local);
bool collide(game::dynent *d, bool spawn);
void setentphysics(int mml, int mmr);
void frame(void);

} /* namespace physics */
} /* namespace q */

