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
  float timeinair;        // time spent in air from jump to jump
  int move:2, strafe:2;   // bwd, fwd, left, right
  uint kleft:1, kright:1; // see input code
  uint kup:1, kdown:1;    // see input code
  uint onfloor:1;         // true if contact with floor
  uint jump:1;            // need to apply jump related physics code
  uint flycam:1;          // no collision / free camera
  string name;            // name of entity
  string team;            // team it belongs to
};

INLINE aabb getaabb(const dynent &d) {
  const auto fmin = d.o-vec3f(d.radius, d.eyeheight, d.radius);
  const auto fmax = d.o+vec3f(d.radius, d.aboveeye, d.radius);
  return aabb(fmin, fmax);
}

extern dynent player;
extern float lastmillis, curtime, speed;
void mousemove(int dx, int dy);
void updateworld(float millis);

} /* namespace game */
} /* namespace q */

