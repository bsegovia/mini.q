/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.cpp -> implements game routines and variables
 -------------------------------------------------------------------------*/
#include "game.hpp"
#include "script.hpp"
#include "physics.hpp"
#include "con.hpp"
#include "sound.hpp"

namespace q {
namespace game {

dynent player;
float lastmillis = 0.f;
float curtime = 1.f;
FVARP(speed, 1.f, 100.f, 1000.f);
FVARP(sensitivity, 0.f, 10.f, 10000.f);
FVARP(sensitivityscale, 1.f, 1.f, 10000.f);
IVARP(invmouse, 0, 0, 1);
IVAR(gamemode, 1, 0, 0);

void resetmovement(dynent *d) {
  d->kleft = 0;
  d->kright = 0;
  d->kup = 0;
  d->kdown = 0;
  d->jump = 0;
  d->strafe = 0;
  d->move = 0;
}

// reset player state not persistent accross spawns
static void spawnstate(dynent *d) {
  resetmovement(d);
  d->vel.x = d->vel.y = d->vel.z = 0;
  d->onfloor = false;
  d->timeinair = 0;
  d->health = 100;
  d->armour = 50;
  d->armourtype = A_BLUE;
  d->quadmillis = 0;
  d->lastattackgun = d->gunselect = GUN_SG;
  d->gunwait = 0;
  d->attacking = false;
  d->lastaction = 0;
  loopi(NUMGUNS) d->ammo[i] = 0;
  d->ammo[GUN_FIST] = 1;
  if (m_noitems) {
    d->gunselect = GUN_RIFLE;
    d->armour = 0;
    if (m_noitemsrail) {
      d->health = 1;
      d->ammo[GUN_RIFLE] = 100;
    } else {
      if (gamemode==12) {
        d->gunselect = GUN_FIST;
        return;
      }  // eihrul's secret "instafist" mode
      d->health = 256;
      if (m_tarena) {
        const int gun1 = rnd(4)+1;
      // TODO  entities::baseammo(d->gunselect = gun1);
        for (;;) {
          const int gun2 = rnd(4)+1;
          if (gun1!=gun2) {
            // TODO entities::baseammo(gun2);
            break;
          }
        }
      } else if (m_arena)    // insta arena
        d->ammo[GUN_RIFLE] = 100;
      else {
        // TODO loopi(4) entities::baseammo(i+1);
        d->gunselect = GUN_CG;
      }
      d->ammo[GUN_CG] /= 2;
    }
  } else
    d->ammo[GUN_SG] = 5;
}

void selfdamage(int damage, int actor, dynent *act) {
  if (player.state!=CS_ALIVE /* TODO || edit::mode() || intermission */)
    return;
  // TODO rr::damageblend(damage);
  // TODO demo::blend(damage);

  // let armour absorb when possible
  int ad = damage*(player.armourtype+1)*20/100;
  if (ad>player.armour)
    ad = player.armour;
  player.armour -= ad;
  damage -= ad;
  const float droll = damage/0.5f;

  // give player a kick depending on amount of damage
  player.ypr.z += player.ypr.z>0.f ? droll :
    (player.ypr.z<0.f ? -droll : (rnd(2) ? droll : -droll));
  if ((player.health -= damage) <= 0) {
    if (actor==-2)
      con::out("you got killed by %s!", act->name);
    else if (actor==-1) {
      // TODO actor = client::getclientnum();
      con::out("you suicided!");
      // TODO client::addmsg(1, 2, SV_FRAGS, --player.frags);
    } else {
#if 0 // TODO
      auto a = getclient(actor);
      if (a) {
        if (isteam(a->team, player.team))
          con::out("you got fragged by a teammate (%s)", a->name);
        else
          con::out("you got fragged by %s", a->name);
      }
#endif
    }
    // TODO showscores(true);
    // TODO client::addmsg(1, 2, SV_DIED, actor);
    player.lifesequence++;
    player.attacking = false;
    player.state = CS_DEAD;
    player.ypr.y = 0.f;
    player.ypr.z = 60.f;
    sound::play(sound::S_DIE1+rnd(2));
    spawnstate(&player);
    player.lastaction = lastmillis;
  } else
    sound::play(sound::S_PAIN6);
}

void entinmap(dynent *d) {
  loopi(100) { // try max 100 times
    const auto dx = (rnd(21)-10)/10.0f*i;  // increasing distance
    const auto dz = (rnd(21)-10)/10.0f*i;
    // TODO proper collision handling
    d->o.x += dx;
    d->o.z += dz;
    return;
  }
  con::out("cannot find entity spawn spot! (%d,%d,%d)",
    int(d->o.x), int(d->o.y), int(d->o.z));
}

// TODO implement spawn point
void spawnplayer(dynent *d) {
  entinmap(d);
  spawnstate(d);
  d->state = CS_ALIVE;
}

dynent::dynent() {
  ZERO(this);
  o.y = eyeheight = 1.80f;
  o.y += 1.f;
  lastmove = lastupdate = lastmillis;
  ypr = vec3f(270.f,0.f,0.f);
  maxspeed = 8.f;
  radius = 0.5f;
  aboveeye = 0.2f;
  lastupdate = lastmillis;
  state = CS_ALIVE;
  spawnstate(this);
}

IVARF(flycam, 0, 0, 1, player.flycam = flycam);

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

