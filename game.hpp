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
  vec3f ypr;              // yaw, pitch, roll
  float lastmove;         // time of last move
  float lastupdate;       // time of last update
  float maxspeed;         // meters per second: 8 for player
  float radius;           // radius of the entity
  float eyeheight;        // height of the eye
  float aboveeye;         // size of what is below the eye
  int move:2, strafe:2;   // bwd, fwd, left, right
  int kleft:1, kright:1;  // see input code
  int kup:1, kdown:1;     // see input code
  int onfloor:1;          // true if contact with floor
  int jumpnext:1;         //
  string name;            // name of entity
  string team;            // team it belongs to
};

extern dynent player;
extern float lastmillis, curtime, speed;
void mousemove(int dx, int dy);
void updateworld(float millis);

} /* namespace game */
} /* namespace q */

