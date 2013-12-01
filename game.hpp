/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.hpp -> exposes game routines and variables
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace game {
extern float lastmillis;
extern float curtime;
extern float speed;
void mousemove(int dx, int dy);
} /* namespace game */
} /* namespace q */

