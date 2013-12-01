/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.cpp -> implements game routines and variables
 -------------------------------------------------------------------------*/
#include "script.hpp"

namespace q {
namespace game {
float lastmillis = 0.f;
float curtime = 1.f;
FVARP(speed, 1.f, 100.f, 1000.f);
void mousemove(int dx, int dy){}
} /* namespace game */
} /* namespace q */

