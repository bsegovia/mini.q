/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.hpp -> exposes game routines and variables
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace game {

// players and monsters
struct dynent {
  vec3f o, vel;           // origin, velocity
  float yaw, pitch, roll; // used as vec3f in one place
  float maxspeed;         // meters per second: 8 for player
};

extern dynent player;
extern float lastmillis, curtime, speed;
void mousemove(int dx, int dy);

} /* namespace game */
} /* namespace q */

