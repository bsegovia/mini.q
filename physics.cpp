/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.cpp -> defines simple physics routine (collision and dynamics)
 -------------------------------------------------------------------------*/
#include "game.hpp"
#include "physics.hpp"
#include "script.hpp"

namespace q {
namespace physics {

bool collide(game::dynent&, bool spawn) { return false; }

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

static void move(game::dynent &p, int moveres, float curtime) {
  vec3f d(zero);
  d.x = float(p.move)*cos(deg2rad(p.ypr.x-90.f));
  d.z = float(p.move)*sin(deg2rad(p.ypr.x-90.f));
  d.x += float(p.strafe)*cos(deg2rad(p.ypr.x-180.f));
  d.z += float(p.strafe)*sin(deg2rad(p.ypr.x-180.f));

  const auto speed = curtime/1000.0f*p.maxspeed;
  const auto friction = p.onfloor ? 6.0f : 30.0f;
  const auto fpsfric = friction/curtime*20.0f;

  // slowly apply friction and direction to velocity, gives a smooth movement
  p.vel *= fpsfric-1.f;
  p.vel += d;
  p.vel /= fpsfric;
  d = p.vel;
  p.o += d*speed;

  // automatically apply smooth roll when strafing
  if (p.strafe==0)
    p.ypr.z = p.ypr.z/(1.f+sqrt(curtime)/25.f);
  else {
    p.ypr.z -= p.strafe*curtime/30.0f;
    if (p.ypr.z>maxroll) p.ypr.z = maxroll;
    if (p.ypr.z<-maxroll) p.ypr.z = -maxroll;
  }
}

void move(game::dynent &p, int moveres) {
  using namespace game;
  const auto frepeat = float(repeat), ratio = curtime/frepeat;
  loopi(repeat) move(p, moveres, i ? ratio : curtime-ratio*(frepeat-1.f));
}

} /* namespace physics */
} /* namespace q */

