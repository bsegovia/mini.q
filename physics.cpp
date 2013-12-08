/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.cpp -> defines simple physics routine (collision and dynamics)
 -------------------------------------------------------------------------*/
#include "game.hpp"
#include "physics.hpp"
#include "script.hpp"

namespace q {
namespace physics {

bool collide(game::dynent*, bool spawn) { return false; }

// physics simulated at 50fps or better
static const float MINFRAMETIME = 20.f;
static float fraction = 0.f;
static int repeat = 0;

FVARP(maxroll, 0.f, 3.f, 20.f);

// optimally schedule physics frames inside the graphics frames
void frame() {
  if (game::curtime >= MINFRAMETIME) {
    const auto faketime = game::curtime + fraction;
    repeat = int(faketime) / int(MINFRAMETIME);
    fraction = faketime - float(repeat) * MINFRAMETIME;
  } else
    repeat = 1;
}

static void move(game::dynent &p, int moveres, bool local, float curtime) {
  vec3f d(zero);
  d.x = float(p.move*cos(deg2rad(p.yaw-90.f)));
  d.y = float(p.move*sin(deg2rad(p.yaw-90.f)));
  d.x += float(p.strafe*cos(deg2rad(p.yaw-180.f)));
  d.y += float(p.strafe*sin(deg2rad(p.yaw-180.f)));

  const auto speed = curtime/1000.0f*p.maxspeed;
  const auto friction = p.onfloor ? 6.0f : 30.0f;
  const auto fpsfric = friction/curtime*20.0f;

  // slowly apply friction and direction to velocity, gives a smooth movement
  p.vel *= fpsfric-1.f;
  p.vel += d;
  p.vel /= fpsfric;
  d = p.vel;
  d *= speed; // d is now frametime based velocity vector

  // automatically apply smooth roll when strafing
  if (p.strafe==0)
    p.roll = p.roll/(1.f+sqrt(curtime)/25.f);
  else {
    p.roll -= p.strafe*curtime/30.0f;
    if (p.roll>maxroll) p.roll = float(maxroll);
    if (p.roll<-maxroll) p.roll = -float(maxroll);
  }
}

void move(game::dynent &p, int moveres, bool local) {
  using namespace game;
  const auto frepeat = float(repeat);
  loopi(repeat)
    move(p, moveres, local, i ?curtime/frepeat : curtime-curtime/frepeat*(frepeat-1.f));
}

} /* namespace physics */
} /* namespace q */

