/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.hpp -> exposes simple physics routine (collision and dynamics)
 -------------------------------------------------------------------------*/
#pragma once
#include "game.hpp"

namespace q {
namespace physics {
void move(game::dynent&, int moveres);
bool collide(game::dynent&, bool spawn);
void frame();
} /* namespace physics */
} /* namespace q */

