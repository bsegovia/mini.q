/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - entities.hpp -> implement shared routines for entities
 - (monsters,players...)
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace game {

vector<game::entity> ents; // all entities
dynent *player1 = newdynent(); // our client
dvector players; // other clients

void cleanentities(void) {
  FREE(player1);
  loopv(players) {
    FREE(players[i]);
    players[i] = NULL;
  }
}

#if 0
static const char *entmdlnames[] = {
  "shells", "bullets", "rockets", "rrounds", "health", "boost",
  "g_armour", "y_armour", "quad",	"teleporter",
};
#endif

static const char *entnames_[] = {
  "none?", "light", "playerstart",
  "shells", "bullets", "rockets", "riflerounds",
  "health", "healthboost", "greenarmour", "yellowarmour", "quaddamage",
  "teleport", "teledest",
  "mapmodel", "monster", "trigger", "jumppad",
  "?", "?", "?", "?", "?"
};
const char *entnames(int which) { return entnames_[which]; }

static int triggertime = 0;

void renderent(entity &e, const char *mdlname, float z, float yaw, int frame = 0, int numf = 1, int basetime = 0, float speed = 10.0f) {
  // TODO rr::rendermodel(mdlname, frame, numf, 0, 1.1f, vec3f(e.x, z, e.y),
  //  yaw, 0, false, 1.0f, speed, 0, basetime);
}

void renderentities(void) {
#if 0 // TODO
  if (lastmillis()>triggertime+1000) triggertime = 0;
  loopv(ents) {
    entity &e = ents[i];
    if (e.type==MAPMODEL) {
      mapmodelinfo &mmi = rr::getmminfo(e.attr2);
      if (!&mmi) continue;
      const vec3f pos(e.x, float(mmi.zoff+e.attr3), e.y);
      // TODO rr::rendermodel(mmi.name, 0, 1, e.attr4, (float)mmi.rad, pos,
      //  (float)((e.attr1+7)-(e.attr1+7)%15), 0, false, 1.0f, 10.0f, mmi.snap);
    } else {
      if (e.type!=CARROT) {
        if (!e.spawned && e.type!=TELEPORT) continue;
        if (e.type<I_SHELLS || e.type>TELEPORT) continue;
        renderent(e, entmdlnames[e.type-I_SHELLS], (float)(1+sin(float(lastmillis())/100.f+e.x+e.y)/20), lastmillis()/10.0f);
      } else switch (e.attr2) {
        case 1:
        case 3: continue;
        case 2:
        case 0:
          if (!e.spawned) continue;
          renderent(e, "carrot", (float)(1+sin(lastmillis()/100.f+e.x+e.y)/20), lastmillis()/(e.attr2 ? 1.0f : 10.0f));
        break;
        case 4: renderent(e, "switch2", 3,      (float)e.attr3*90, (!e.spawned && !triggertime) ? 1  : 0, (e.spawned || !triggertime) ? 1 : 2,  triggertime, 1050.0f);  break;
        case 5: renderent(e, "switch1", -0.15f, (float)e.attr3*90, (!e.spawned && !triggertime) ? 30 : 0, (e.spawned || !triggertime) ? 1 : 30, triggertime, 35.0f); break;
      }
    }
  }
#endif
}

static const struct itemstat { int add, max, sound; } itemstats[] = {
  {   10,    50, sound::ITEMAMMO},
  {   20,   100, sound::ITEMAMMO},
  {    5,    25, sound::ITEMAMMO},
  {    5,    25, sound::ITEMAMMO},
  {   25,   100, sound::ITEMHEALTH},
  {   50,   200, sound::ITEMHEALTH},
  {  100,   100, sound::ITEMARMOUR},
  {  150,   150, sound::ITEMARMOUR},
  {20000, 30000, sound::ITEMPUP}
};

void baseammo(int gun) { player1->ammo[gun] = itemstats[gun-1].add*2; };

// these two functions are called when the server acknowledges that you really
// picked up the item (in client::multiplayer someone may grab it before you).
void radditem(int i, int &v) {
  const itemstat &is = itemstats[ents[i].type-I_SHELLS];
  ents[i].spawned = false;
  v += is.add;
  if (v>is.max) v = is.max;
  sound::playc(is.sound);
}

void realpickup(int n, game::dynent *d) {
  switch (ents[n].type) {
    case I_SHELLS:  radditem(n, d->ammo[1]); break;
    case I_BULLETS: radditem(n, d->ammo[2]); break;
    case I_ROCKETS: radditem(n, d->ammo[3]); break;
    case I_ROUNDS:  radditem(n, d->ammo[4]); break;
    case I_HEALTH:  radditem(n, d->health);  break;
    case I_BOOST:   radditem(n, d->health);  break;
    case I_GREENARMOUR:
      radditem(n, d->armour);
      d->armourtype = A_GREEN;
    break;
    case I_YELLOWARMOUR:
      radditem(n, d->armour);
      d->armourtype = A_YELLOW;
    break;
    case I_QUAD:
      radditem(n, d->quadmillis);
      con::out("you got the quad!");
    break;
  }
}

// these functions are called when the client touches the item

void additem(int i, int &v, int spawnsec) {
  if (v<itemstats[ents[i].type-I_SHELLS].max) { // don't pick up if not needed
    client::addmsg(1, 3, SV_ITEMPICKUP, i, m_classicsp ? 100000 : spawnsec);    // first ask the server for an ack
    ents[i].spawned = false;                                            // even if someone else gets it first
  }
}

void teleport(int n, game::dynent *d) { // also used by monsters
#if 0 // TODO
  int e = -1, tag = ents[n].attr1, beenhere = -1;
  for (;;) {
    e = world::findentity(TELEDEST, e+1);
    if (e==beenhere || e<0) {
      con::out("no teleport destination for tag %d", tag);
      return;
    }
    if (beenhere<0) beenhere = e;
    if (ents[e].attr2==tag) {
      d->o = vec3f(ents[e].x, ents[e].y, ents[e].z);
      d->vel = zero;
      d->ypr.x = ents[e].attr1;
      d->ypr.y = 0.f;
      entinmap(d);
      sound::playc(sound::TELEPORT);
      break;
    }
  }
#endif
}

void pickup(int n, game::dynent *d) {
  int np = 1;
  loopv(players) if (players[i]) np++;
  np = np<3 ? 4 : (np>4 ? 2 : 3);         // spawn times are dependent on number of players
  int ammo = np*2;
  switch (ents[n].type) {
    case I_SHELLS:  additem(n, d->ammo[1], ammo); break;
    case I_BULLETS: additem(n, d->ammo[2], ammo); break;
    case I_ROCKETS: additem(n, d->ammo[3], ammo); break;
    case I_ROUNDS:  additem(n, d->ammo[4], ammo); break;
    case I_HEALTH:  additem(n, d->health,  np*5); break;
    case I_BOOST:   additem(n, d->health,  60);   break;
    case I_GREENARMOUR:
      // (100h/100g only absorbs 166 damage)
      if (d->armourtype==A_YELLOW && d->armour>66) break;
      additem(n, d->armour, 20);
    break;
    case I_YELLOWARMOUR:
      additem(n, d->armour, 20);
    break;
    case I_QUAD:
      additem(n, d->quadmillis, 60);
    break;
    case CARROT:
      ents[n].spawned = false;
      triggertime = int(lastmillis());
      // TODO world::trigger(ents[n].attr1, ents[n].attr2, false);  // needs to go over server for client::multiplayer
    break;
    case TELEPORT: {
      static int lastteleport = 0;
      if (lastmillis()-lastteleport<500) break;
      lastteleport = int(lastmillis());
      teleport(n, d);
    }
    break;
    case JUMPPAD: {
      static int lastjumppad = 0;
      if (lastmillis()-lastjumppad<300) break;
      lastjumppad = int(lastmillis());
      vec3f v((int)(char)ents[n].attr3/10.0f, (int)(char)ents[n].attr2/10.0f, ents[n].attr1/10.0f);
      player1->vel.z = 0;
      player1->vel += v;
      sound::playc(sound::JUMPPAD);
    }
    break;
  }
}

void checkitems(void) {
  if (edit::mode()) return;
  loopv(ents) {
    entity &e = ents[i];
    if (e.type==NOTUSED) continue;
    if (!ents[i].spawned && e.type!=TELEPORT && e.type!=JUMPPAD) continue;
    const vec3f v(float(e.x), float(e.y), player1->eyeheight);
    const float dist = distance(player1->o, v);
    if (dist<(e.type==TELEPORT ? 4 : 2.5))
      pickup(i, player1);
  }
}

void checkquad(int time) {
  if (player1->quadmillis && (player1->quadmillis -= time)<0) {
    player1->quadmillis = 0;
    sound::playc(sound::PUPOUT);
    con::out("quad damage is over");
  }
}

void putitems(u8 *&p) { // puts items in network stream and also spawns them locally
  loopv(ents)
    if ((ents[i].type>=I_SHELLS && ents[i].type<=I_QUAD) ||
        ents[i].type==CARROT) {
    putint(p, i);
    ents[i].spawned = true;
  }
}

void resetspawns(void) { loopv(ents) ents[i].spawned = false; }
void setspawn(u32 i, bool on) { if (i<(u32)ents.length()) ents[i].spawned = on; }
} /* namespace game */
} /* namespace q */

