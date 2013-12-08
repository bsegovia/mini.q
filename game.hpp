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
  dynent();
  vec3f o, vel;           // origin, velocity
  float yaw, pitch, roll; // used as vec3f in one place
  float lastmove;         // time of last move
  float lastupdate;       // time of last update
  float maxspeed;         // meters per second: 8 for player
  float radius;           // radius of the entity
  float eyeheight;        // height of the eye
  float aboveeye;         // size of what is below the eye
  int move, strafe;       // bwd, fwd, left, right
  bool kleft, kright;     // see input code
  bool kup, kdown;        // see input code
  string name;            // name of entity
  string team;            // team it belongs to
};

extern dynent player;
extern float lastmillis, curtime, speed;
void mousemove(int dx, int dy);

} /* namespace game */
} /* namespace q */

