/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.cpp -> implements game routines and variables
 -------------------------------------------------------------------------*/
#include "script.hpp"
#include "physics.hpp"
#include "game.hpp"

namespace q {
namespace game {

dynent player;
float lastmillis = 0.f;
float curtime = 1.f;
FVARP(speed, 1.f, 100.f, 1000.f);
FVARP(sensitivity, 0.f, 10.f, 10000.f);
FVARP(sensitivityscale, 1.f, 1.f, 10000.f);
IVARP(invmouse, 0, 0, 1);

dynent::dynent() {
  o = vel = zero;
  o.y = eyeheight = 1.80f;
  o.y += 1.f;
  ypr = vec3f(0.f,0.f,0.f);
  lastmove = lastupdate = lastmillis;
  maxspeed = 8.f;
  radius = 0.5f;
  aboveeye = 0.2f;
  kleft = kright = kup = kdown = 0;
  onfloor = move = strafe = jump = flycam = 0;
  gun = 0;
  name[0] = team[0] = '\0';
}
IVAR(flycam, 0, 0, 1);

void fixplayerrange(void) {
  const float MAXPITCH = 90.0f;
  clamp(player.ypr.y, -MAXPITCH, MAXPITCH);
  while (player.ypr.x<0.0f) player.ypr.x += 360.0f;
  while (player.ypr.x>=360.0f) player.ypr.x -= 360.0f;
}

void mousemove(int dx, int dy) {
  const float SENSF = 33.0f;
  player.ypr.x += (float(dx)/SENSF)*(sensitivity/sensitivityscale);
  player.ypr.y -= (float(dy)/SENSF)*(sensitivity/sensitivityscale)*(invmouse?-1.f:1.f);
  fixplayerrange();
}

#define DIRECTION(name,v,d,s,os) \
static void name(const int &isdown) { \
  player.s = isdown; \
  player.v = isdown ? d : (player.os ? -(d) : 0); \
  player.lastmove = lastmillis; \
} CMD(name, "d");
DIRECTION(backward, move,   -1, kdown,  kup);
DIRECTION(forward,  move,   +1, kup,    kdown);
DIRECTION(left,     strafe, +1, kleft,  kright);
DIRECTION(right,    strafe, -1, kright, kleft);
#undef DIRECTION

static void jump(const int &isdown) {
  player.jump = isdown;
}
CMD(jump, "d");

static const int moveres = 20;
void updateworld(float millis) {
  curtime = millis - lastmillis;
  physics::frame();
  physics::move(player, moveres);
  lastmillis = millis;
}
} /* namespace game */
} /* namespace q */

