/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.cpp -> defines simple physics routine (collision and dynamics)
 -------------------------------------------------------------------------*/
#include "game.hpp"
#include "physics.hpp"
#include "script.hpp"

namespace q {
namespace physics {

INLINE bool collide(const aabb &box) {
  return box.pmin.y < 0.f;
}
bool collide(game::dynent &d, bool spawn) {
  return collide(game::getaabb(d));
}

// physics simulated at 50fps or better
static const float MINFRAMETIME = 20.f;
static float fraction = 0.f;
static int repeat = 0;

FVARP(maxroll, 0.f, 3.f, 20.f);
FVAR(gravity, 1.f, 20.f, 100.f);

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

  const auto speed = 4.f*curtime/1000.0f*p.maxspeed;
  const auto friction = p.onfloor ? 6.0f : 30.0f;
  const auto fpsfric = friction/curtime*20.0f;

  // slowly apply friction and direction to velocity, gives a smooth movement
  p.vel *= fpsfric-1.f;
  p.vel += d;
  p.vel /= fpsfric;
  d = p.vel;
  d *= speed; // d is now frametime based velocity vector

  if (p.flycam) {
    d.x *= float(cos(deg2rad(p.ypr.y)));
    d.y  = -float(p.move*sin(deg2rad(p.ypr.y)));
    d.z *= float(cos(deg2rad(p.ypr.y)));
  }

  // we apply the velocity vector directly if we are flying around
  if (p.flycam) {
    p.o += d;
    if (p.jump) {
      p.vel.y += 2.f;
      p.jump = 0;
    }
  }
  // we have to handle collision (here just with plane y==0)
  else {
    // printf("\rjump %d                       ", p.jump);
    // update *velocity* based on action and position in the world
    if (p.onfloor) {
      if (p.jump) {
        p.jump = 0;
        p.vel.y = 1.7f;
      }
      p.timeinair = 0.f;
    } else
      p.timeinair += curtime;

    // update *position* with discrete step collision
    const float f = 1.0f/float(moveres);
    float dropf = ((gravity-1.f)+p.timeinair/15.0f); // incorrect, but works fine
    const float drop = dropf*curtime/gravity/100./float(moveres); // at high fps, gravity kicks in too fast

    loopi(moveres) {
      // try to apply gravity
      p.o.y -= drop;
      if (collide(p, false)) {
        p.o.y += drop;
        p.onfloor = true;
      } else
        p.onfloor = false;
      p.o += f*d;
    }
  }

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

