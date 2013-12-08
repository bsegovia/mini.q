/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.cpp -> implements game routines and variables
 -------------------------------------------------------------------------*/
#include "script.hpp"
#include "game.hpp"

namespace q {
namespace game {

dynent player;
float lastmillis = 0.f;
float curtime = 1.f;
FVARP(speed, 1.f, 100.f, 1000.f);
IVARP(sensitivity, 0, 10, 10000);
IVARP(sensitivityscale, 1, 1, 10000);
IVARP(invmouse, 0, 0, 1);

dynent::dynent() {
  o = vel = zero;
  yaw = 270.f;
  roll = pitch = 0.f;
  lastmove = lastupdate = lastmillis;
  maxspeed = 8.f;
  radius = 0.5f;
  eyeheight = 1.80f;
  aboveeye = 0.2f;
  move = strafe = 0;
  kleft = kright = kup = kdown = false;
  name[0] = team[0] = '\0';
}

void fixplayerrange(void) {
  const float MAXPITCH = 90.0f;
  if (player.pitch>MAXPITCH) player.pitch = MAXPITCH;
  if (player.pitch<-MAXPITCH) player.pitch = -MAXPITCH;
  while (player.yaw<0.0f) player.yaw += 360.0f;
  while (player.yaw>=360.0f) player.yaw -= 360.0f;
}

void mousemove(int dx, int dy) {
  const float SENSF = 33.0f; // try match quake sens
  player.yaw += (dx/SENSF)*(sensitivity/float(sensitivityscale));
  player.pitch -= (dy/SENSF)*(sensitivity/float(sensitivityscale))*(invmouse?-1:1);
  fixplayerrange();
}

#define DIRECTION(name,v,d,s,os) \
static void name(const int &isdown) { \
  player.s = isdown!=0; \
  player.v = isdown!=0 ? d : (player.os ? -(d) : 0); \
  player.lastmove = lastmillis; \
}\
CMD(name, "d");
DIRECTION(backward, move,   -1, kdown,  kup);
DIRECTION(forward,  move,    1, kup,    kdown);
DIRECTION(left,     strafe,  1, kleft,  kright);
DIRECTION(right,    strafe, -1, kright, kleft);
#undef DIRECTION

} /* namespace game */
} /* namespace q */

