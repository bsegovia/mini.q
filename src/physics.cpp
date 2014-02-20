/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - physics.cpp -> implements all physics routines
 -------------------------------------------------------------------------*/
// no physics books were hurt nor consulted in the construction of this code.
// All physics computations and constants were invented on the fly and simply
// tweaked until they "felt right", and have no basis in reality.  Collision
// detection is simplistic but very robust (uses discrete steps at fixed fps).
#include "mini.q.hpp"

namespace q {
namespace physics {

// collide with player or monster
static bool plcollide(const aabb &box, const game::dynent *o) {
  if (o->state!=CS_ALIVE) return true;
  const auto oaabb = getaabb(o);
  return !intersect(box, oaabb);
}

// collide with a map model
static bool mmcollide(const aabb &box) {
#if 0 // TODO
  loopv(game::ents) {
    game::entity &e = game::ents[i];
    if (e.type != game::MAPMODEL) continue;
    const game::mapmodelinfo &mmi = rr::getmminfo(e.attr2);
    if (!&mmi || !mmi.h) continue;
    const auto rad = float(mmi.rad), h = float(mmi.h);
    const auto emin = vec3f(float(e.x),float(e.y),float(e.z))-vec3f(rad,rad,0.f);
    const auto emax = vec3f(float(e.x),float(e.y),float(e.z))+vec3f(rad,rad,h);
    const aabb eaabb(emin,emax);
    if (intersect(box, eaabb)) return false;
  }
#endif
  return true;
}
#if 0
// get bounding volume of the given deformed cube
INLINE aabb getaabb(vec3i xyz) {
  aabb box(FLT_MAX,-FLT_MAX);
  loopi(8) {
    const auto v = world::getpos(xyz+cubeiverts[i]);
    box.pmin = min(v, box.pmin);
    box.pmax = max(v, box.pmax);
  }
  return box;
}
#endif

// collide with the map
static bool mapcollide(const aabb &box) {
  return box.pmin.y >= 0.f;
}

// all collision happens here. spawn is a dirty side effect used in
// spawning. drop & rise are supplied by the physics below to indicate
// gravity/push for current mini-timestep
bool collide(game::dynent *d, bool spawn) {
  const auto box = getaabb(d);

  // collide with map
  if (!mapcollide(box)) return false;

  // collide with other players
  loopv(game::players) {
    game::dynent *o = game::players[i];
    if (!o || o==d) continue;
    if (!plcollide(box, o)) return false;
  }
  if (d!=game::player1)
    if (!plcollide(box, game::player1)) return false;
  auto &v = game::getmonsters();
  loopv(v) if (!plcollide(box, v[i])) return false;

  // collide with map models
  if (!mmcollide(box)) return false;
  return true;
}

VARP(maxroll, 0, 3, 20);

static int physicsfraction = 0, physicsrepeat = 0;
static const int MINFRAMETIME = 20; // physics simulated at 50fps or better

// optimally schedule physics frames inside the graphics frames
void frame(void) {
  if (game::curtime()>=MINFRAMETIME) {
    int faketime = game::curtime()+physicsfraction;
    physicsrepeat = faketime/MINFRAMETIME;
    physicsfraction = faketime-physicsrepeat*MINFRAMETIME;
  } else
    physicsrepeat = 1;
}

// main physics routine, moves a player/monster for a curtime step.
// moveres indicated the physics precision (which is lower for monsters and
// client::multiplayer prediction).
// local is false for client::multiplayer prediction
void moveplayer(game::dynent *pl, int moveres, bool local, int curtime) {
  const bool water = false;//world::waterlevel() > pl->o.z-0.5f;
  const bool floating = (edit::mode() && local) || pl->state==CS_EDITING;

  vec3f d; // vector of direction we ideally want to move in
  d.x = float(pl->move*cos(deg2rad(pl->ypr.x-90.f)));
  d.z = float(pl->move*sin(deg2rad(pl->ypr.x-90.f)));
  d.y = 0.f;

  if (floating || water) {
    d.x *= float(cos(deg2rad(pl->ypr.y)));
    d.z *= float(cos(deg2rad(pl->ypr.y)));
    d.y  = float(pl->move*sin(deg2rad(pl->ypr.y)));
  }

  d.x += float(pl->strafe*cos(deg2rad(pl->ypr.x-180.f)));
  d.z += float(pl->strafe*sin(deg2rad(pl->ypr.x-180.f)));

  const float speed = curtime/(water? 2000.0f : 1000.0f)*pl->maxspeed;
  const float friction = water ? 20.0f : (pl->onfloor||floating ? 6.0f : 30.0f);
  const float fpsfric = friction/curtime*20.0f;

  // slowly apply friction and direction to velocity, gives a smooth movement
  pl->vel *= fpsfric-1.f;
  pl->vel += d;
  pl->vel /= fpsfric;
  d = pl->vel;
  d *= speed; // d is now frametime based velocity vector

  pl->blocked = false;
  pl->moving = true;

  if (floating) { // just apply velocity
    pl->o += d;
    if (pl->jumpnext) {
      pl->jumpnext = false;
      pl->vel.y = 2.f;
    }
  } else { // apply velocity with collision
    if (pl->onfloor || water) {
      if (pl->jumpnext) {
        pl->jumpnext = false;
        pl->vel.y = 1.7f; // physics impulse upwards
        if (water) { // dampen velocity change even harder, gives correct water feel
          pl->vel.x /= 8.f;
          pl->vel.z /= 8.f;
        }
        if (local)
          sound::playc(sound::JUMP);
        else if (pl->monsterstate)
          sound::play(sound::JUMP, &pl->o);
      } else if (pl->timeinair>800) { // if we land after long time must have been a high jump, add sound
        if (local)
          sound::playc(sound::LAND);
        else if (pl->monsterstate)
          sound::play(sound::LAND, &pl->o);
      }
      pl->timeinair = 0;
    } else
      pl->timeinair += curtime;

    const float gravity = 20.f;
    const float f = 1.0f/float(moveres);
    float dropf = ((gravity-1.f)+pl->timeinair/15.0f); // incorrect, but works fine
    if (water) { // float slowly down in water
      dropf = 5.f;
      pl->timeinair = 0;
    }
    const float drop = dropf*curtime/gravity/100/moveres; // at high fps, gravity kicks in too fast

    loopi(moveres) { // discrete steps collision detection & sliding
      // try to apply gravity
      pl->o.y -= drop;
      if (!collide(pl, false)) {
        pl->o.y += drop;
        pl->onfloor = true;
      } else
        pl->onfloor = false;

      // try move forward
      pl->o += f*d;
      if (collide(pl, false)) continue;

      // player stuck, try slide along all axis
      bool successful = false;
      pl->blocked = true;
      loopi(3) {
        pl->o[i] -= f*d[i];
        if (collide(pl, false)) {
          successful = true;
          d[i] = 0.f;
          break;
        }
        pl->o[i] += f*d[i];
      }
      if (successful) continue;

      // try just dropping down
      pl->moving = false;
      pl->o.x -= f*d.x;
      pl->o.z -= f*d.z;
      if (collide(pl, false)) {
        d.x = d.z = 0.f;
        continue;
      }
      pl->o.y -= f*d.y;
      break;
    }
  }
  pl->outsidemap = false;

  // automatically apply smooth roll when strafing
  if (pl->strafe==0)
    pl->ypr.z = pl->ypr.z/(1.f+float(sqrt(float(curtime)))/25.f);
  else {
    pl->ypr.z += pl->strafe*curtime/-30.0f;
    if (pl->ypr.z>maxroll) pl->ypr.z = float(maxroll);
    if (pl->ypr.z<-maxroll) pl->ypr.z = -float(maxroll);
  }

  // play sounds on water transitions
  if (!pl->inwater && water) {
    sound::play(sound::SPLASH2, &pl->o);
    pl->vel.y = 0.f;
  } else if (pl->inwater && !water)
    sound::play(sound::SPLASH1, &pl->o);
  pl->inwater = water;
}

void moveplayer(game::dynent *pl, int moveres, bool local) {
  loopi(physicsrepeat)
    moveplayer(pl, moveres, local,
      i ? game::curtime()/physicsrepeat :
          game::curtime()-game::curtime()/physicsrepeat*(physicsrepeat-1));
}
} /* namespace physics */
} /* namespace q */

