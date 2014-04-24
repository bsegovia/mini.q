/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - demo.cpp -> implements demo record / play back
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include <zlib.h>

// loading and saving of savegames & demos, dumps the spawn state of all
// mapents, the full state of all dynents (monsters + player)
namespace q {
namespace demo {

static gzFile f = NULL;
static game::dvector playerhistory;
static bool demorecording = false;
static bool demoloading = false;
static bool demoplayback = false;
static int democlientnum = 0;

static void start(void);

static void gzput(int i) { gzputc(f, i); }
static void gzputi(int i) { gzwrite(f, &i, sizeof(int)); }
static void gzputv(vec3f &v) { gzwrite(f, &v, sizeof(vec3f)); }
static void gzcheck(int a, int b) {
  if (a!=b) sys::fatal("savegame file corrupt (short)");
}
static int gzget(void) {
  char c = gzgetc(f);
  return c;
}
static int gzgeti(void) {
  int i;
  gzcheck(gzread(f, &i, sizeof(int)), sizeof(int));
  return i;
}
static void gzgetv(vec3f &v) { gzcheck(gzread(f, &v, sizeof(vec3f)), sizeof(vec3f)); }

bool playing(void) { return demoplayback; }
int clientnum(void) { return democlientnum; }

void stop(void) {
  if (f) {
    if (demorecording) gzputi(-1);
    gzclose(f);
  }
  f = NULL;
  demorecording = false;
  demoplayback = false;
  demoloading = false;
  loopv(playerhistory) game::zapdynent(playerhistory[i]);
  playerhistory.setsize(0);
}

void stopifrecording(void) { if (demorecording) stop(); }

static void savestate(char *fn) {
  stop();
  f = gzopen(fn, "wb9");
  if (!f) { con::out("could not write %s", fn); return; }
  gzwrite(f, (void *)"CUBESAVE", 8);
  gzputc(f, sys::islittleendian());
  gzputi(SAVEGAMEVERSION);
  gzputi(sizeof(game::dynent));
  gzwrite(f, (const voidp) game::getclientmap(), MAXDEFSTR);
  gzputi(game::mode());
  gzputi(game::ents.length());
  loopv(game::ents) gzputc(f, game::ents[i].spawned);
  gzwrite(f, game::player1, sizeof(game::dynent));
  game::dvector &monsters = game::getmonsters();
  gzputi(monsters.length());
  loopv(monsters) gzwrite(f, monsters[i], sizeof(game::dynent));
  gzputi(game::players.length());
  loopv(game::players) {
    gzput(game::players[i]==NULL);
    gzwrite(f, game::players[i], sizeof(game::dynent));
  }
}

static void savegame(const char *name) {
  if (!m_classicsp) {
    con::out("can only save classic sp games");
    return;
  }
  fixedstring fn(fmt,"savegames/%s.csgz", name);
  savestate(fn.c_str());
  stop();
  con::out("wrote %s", fn.c_str());
}
CMD(savegame);

static void loadstate(char *fn) {
  fixedstring buf, mapname;
  stop();
  if (client::multiplayer()) return;
  f = gzopen(fn, "rb9");
  if (!f) { con::out("could not open %s", fn); return; }

  gzread(f, buf.c_str(), 8);
  if (strncmp(buf.c_str(), "CUBESAVE", 8)) goto out;
  if (gzgetc(f)!=sys::islittleendian()) goto out;     // not supporting save->load accross incompatible architectures simpifies things a LOT
  if (gzgeti()!=SAVEGAMEVERSION || gzgeti()!=sizeof(game::dynent)) goto out;
  gzread(f, mapname.c_str(), MAXDEFSTR);
  game::setnextmode(gzgeti());
  // continue below once map has been loaded and client & server have updated
  client::changemap(mapname.c_str());
  return;
out:
  con::out("aborting: savegame/demo from a different version of cube or cpu architecture");
  stop();
}

static void loadgame(char *name) {
  fixedstring fn(fmt,"savegames/%s.csgz", name);
  loadstate(fn.c_str());
}
CMD(loadgame);

static void loadgameout(void) {
  stop();
  con::out("loadgame incomplete: savegame from a different version of this map");
}

void loadgamerest(void) {
  if (demoplayback || !f) return;
  if (gzgeti()!=game::ents.length()) return loadgameout();
  loopv(game::ents) {
    game::ents[i].spawned = gzgetc(f)!=0;
    // TODO if (game::ents[i].type==game::CARROT && !game::ents[i].spawned)
      // world::trigger(game::ents[i].attr1, game::ents[i].attr2, true);
  }
  server::restoreserverstate(game::ents);

  gzread(f, game::player1, sizeof(game::dynent));
  game::player1->lastaction = int(game::lastmillis());

  int nmonsters = gzgeti();
  game::dvector &monsters = game::getmonsters();
  if (nmonsters!=monsters.length()) return loadgameout();
  loopv(monsters) {
    gzread(f, monsters[i], sizeof(game::dynent));
    monsters[i]->enemy = game::player1; // lazy, could save id of enemy instead
    monsters[i]->lastaction = monsters[i]->trigger = int(game::lastmillis())+500; // also lazy, but no real noticable effect on game
    if (monsters[i]->state==CS_DEAD) monsters[i]->lastaction = 0;
  }
  game::restoremonsterstate();

  int nplayers = gzgeti();
  loopi(nplayers) if (!gzget()) {
    game::dynent *d = game::getclient(i);
    assert(d);
    gzread(f, d, sizeof(game::dynent));
  }

  con::out("savegame restored");
  if (demoloading) start(); else stop();
}

// demo functions
static int starttime = 0;
static int playbacktime = 0;
static int ddamage, bdamage;
static vec3f dorig;

static void record(const char *name) {
  if (m_sp) {
    con::out("cannot record singleplayer games");
    return;
  }
  int cn = client::getclientnum();
  if (cn<0) return;
  fixedstring fn(fmt,"demos/%s.cdgz", name);
  savestate(fn.c_str());
  gzputi(cn);
  con::out("started recording demo to %s", fn.c_str());
  demorecording = true;
  starttime = int(game::lastmillis());
  ddamage = bdamage = 0;
}
CMD(record);

void damage(int damage, vec3f &o) { ddamage = damage; dorig = o; }
void blend(int damage) { bdamage = damage; }

void incomingdata(u8 *buf, int len, bool extras) {
  if (!demorecording) return;
  gzputi(int(game::lastmillis())-starttime);
  gzputi(len);
  gzwrite(f, buf, len);
  gzput(extras);
  if (extras) {
    gzput(game::player1->gunselect);
    gzput(game::player1->lastattackgun);
    gzputi(game::player1->lastaction-starttime);
    gzputi(game::player1->gunwait);
    gzputi(game::player1->health);
    gzputi(game::player1->armour);
    gzput(game::player1->armourtype);
    loopi(game::NUMGUNS) gzput(game::player1->ammo[i]);
    gzput(game::player1->state);
    gzputi(bdamage);
    bdamage = 0;
    gzputi(ddamage);
    if (ddamage) { gzputv(dorig); ddamage = 0; }
    // FIXME: add all other client state which is not send through the network
  }
}

static void demo(const char *name) {
  fixedstring fn(fmt,"demos/%s.cdgz", name);
  loadstate(fn.c_str());
  demoloading = true;
}
CMD(demo);

static void stopreset(void) {
  con::out("demo stopped (%d msec elapsed)", game::lastmillis()-starttime);
  stop();
  loopv(game::players) game::zapdynent(game::players[i]);
  client::disconnect(0, 0);
}

VAR(demoplaybackspeed, 10, 100, 1000);
static int scaletime(int t) {
  return (int)(t*(100.0f/demoplaybackspeed))+starttime;
}

static void readdemotime() {
  if (gzeof(f) || (playbacktime = gzgeti())==-1) {
    stopreset();
    return;
  }
  playbacktime = scaletime(playbacktime);
}

static void start(void) {
  democlientnum = gzgeti();
  demoplayback = true;
  starttime = int(game::lastmillis());
  con::out("now playing demo");
  game::dynent *d = game::getclient(democlientnum);
  assert(d);
  *d = *game::player1;
  readdemotime();
}

VAR(demodelaymsec, 0, 120, 500);

// spline interpolation
static void catmulrom(const vec3f &z, const vec3f &a, const vec3f &b, const vec3f &c, float s, vec3f &dst) {
  const float s2 = s*s;
  const float s3 = s*s2;
  const vec3f t  = (-2.f*s3 + 3.f*s2)*b;
  const vec3f t1 = (s3-2.f*s2+s)*0.5f*(b-z);
  const vec3f t2 = (s3-s2)*0.5f*(c-a);
  dst = (2.f*s3 - 3.f*s2 + 1.f)*a + t + t1 + t2;
}

static void fixwrap(game::dynent *a, game::dynent *b) {
  while (b->ypr.x-a->ypr.x>180)  a->ypr.x += 360;
  while (b->ypr.x-a->ypr.x<-180) a->ypr.x -= 360;
}

void playbackstep(void) {
  while (demoplayback && game::lastmillis()>=playbacktime) {
    int len = gzgeti();
    if (len<1 || len>MAXTRANS) {
      con::out("error: huge packet during demo play (%d)", len);
      stopreset();
      return;
    }
    u8 buf[MAXTRANS];
    gzread(f, buf, len);
    client::localservertoclient(buf, len);  // update game state

    game::dynent *target = game::players[democlientnum];
    assert(target);

    int extras;
    if ((extras = gzget())) { // read additional client side state not present in normal network stream
      target->gunselect = gzget();
      target->lastattackgun = gzget();
      target->lastaction = scaletime(gzgeti());
      target->gunwait = gzgeti();
      target->health = gzgeti();
      target->armour = gzgeti();
      target->armourtype = gzget();
      loopi(game::NUMGUNS) target->ammo[i] = gzget();
      target->state = gzget();
      target->lastmove = playbacktime;
      // if ((bdamage = gzgeti()) != 0) rr::damageblend(bdamage);
      if ((ddamage = gzgeti()) != 0) {
        gzgetv(dorig);
        rr::particle_splash(rr::PT_BLOOD_SPATS, ddamage, 1000, dorig);
      }
      // FIXME: set more client state here
    }

    // insert latest copy of player into history
    if (extras && (playerhistory.empty() || playerhistory.last()->lastupdate!=playbacktime)) {
      game::dynent *d = game::newdynent();
      *d = *target;
      d->lastupdate = playbacktime;
      playerhistory.add(d);
      if (playerhistory.length()>20) {
        game::zapdynent(playerhistory[0]);
        playerhistory.remove(0);
      }
    }

    readdemotime();
  }

  if (demoplayback) {
    int itime = int(game::lastmillis())-demodelaymsec;
    loopvrev(playerhistory) if (playerhistory[i]->lastupdate<itime) { // find 2 positions in history that surround interpolation time point
      game::dynent *a = playerhistory[i];
      game::dynent *b = a;
      if (i+1<playerhistory.length()) b = playerhistory[i+1];
      *game::player1 = *b;
      if (a!=b) { // interpolate pos & angles
        game::dynent *c = b;
        if (i+2<playerhistory.length()) c = playerhistory[i+2];
        game::dynent *z = a;
        if (i-1>=0) z = playerhistory[i-1];
        float bf = (itime-a->lastupdate)/(float)(b->lastupdate-a->lastupdate);
        fixwrap(a, game::player1);
        fixwrap(c, game::player1);
        fixwrap(z, game::player1);
        const float dist = distance(z->o,c->o);
        if (dist<16.f) { // if teleport or spawn, dont't interpolate
          catmulrom(z->o, a->o, b->o, c->o, bf, game::player1->o);
          // catmulrom(*(vec3f*)&z->ypr.x, *(vec3f*)&a->ypr.x, *(vec3f*)&b->ypr.x, *(vec3f*)&c->ypr.x, bf, *(vec3f *)&game::player1->ypr.x);
          vec3f dstangle;
          catmulrom(z->ypr, a->ypr, b->ypr, c->ypr, bf, dstangle);
          game::player1->ypr.x = dstangle.x;
          game::player1->ypr.y = dstangle.y;
          game::player1->ypr.z = dstangle.z;
        }
        game::fixplayer1range();
      }
      break;
    }
  }
}

static void stopn() {
  if (demoplayback) stopreset(); else stop();
  con::out("demo stopped");
}
CMDN(stop, stopn);
} /* namespace demo */
} /* namespace q */

