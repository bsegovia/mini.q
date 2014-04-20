/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.cpp -> implements game routines and variables
 -------------------------------------------------------------------------*/
#if !defined(STANDALONE)
#include "mini.q.hpp"
#include "enet/enet.h"

namespace q {
namespace game {

static double lastmillis_ = 0;
static int nextmode_ = 0;
static vec3f worldpos_;
static double curtime_ = 10;
VAR(gamemode, 1, 0, 0);

GLOBAL_VAR(worldpos, worldpos_, const vec3f&);
GLOBAL_VAR(mode, gamemode, int);
GLOBAL_VAR(nextmode, nextmode_, int);
GLOBAL_VAR(curtime, curtime_, double);
GLOBAL_VAR(lastmillis, lastmillis_, double);

VARP(sensitivity, 0, 10, 10000);
VARP(sensitivityscale, 1, 1, 10000);
VARP(invmouse, 0, 0, 1);

static void moden(int n) {client::addmsg(1,2,SV_GAMEMODE,nextmode_=n);}
CMDN(mode, moden);

static bool intermission = false;
static fixedstring clientmap;

const char *getclientmap() { return clientmap.c_str(); }

INLINE mat4x4f viewportxfm(float w, float h) {
  return mat4x4f(vec4f(2.f/w,0.f,  0.f,0.f),
                 vec4f(0.f,  2.f/h,0.f,0.f),
                 vec4f(0.f,  0.f,  2.f,0.f),
                 vec4f(-1.f,-1.f, -1.f,1.f));
}

mat4x4f mvmat, pmat, mvpmat, invmvpmat, dirinvmvpmat;

// compute player1 matrices
// XXX bad code. we should invert transform instead of calling "inverse".
void setmatrices(float fovy, float farplane, float w, float h) {
  const auto aspect = w / h;
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  pmat = ogl::matrix(ogl::PROJECTION);
  ogl::matrixmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::rotate(player1->ypr.z, vec3f(0.f,0.f,1.f));
  ogl::rotate(player1->ypr.y, vec3f(1.f,0.f,0.f));
  ogl::rotate(player1->ypr.x, vec3f(0.f,1.f,0.f));
  const auto dirmv = ogl::matrix(ogl::MODELVIEW);
  ogl::translate(-player1->o);
  mvmat = ogl::matrix(ogl::MODELVIEW);
  mvpmat = pmat*mvmat;
  invmvpmat = mvpmat.inverse() * viewportxfm(w, h);
  dirinvmvpmat = (pmat*dirmv).inverse() * viewportxfm(w, h);
}

// creation of scoreboard pseudo-menu
static bool scoreson = false;
static void showscores(bool on) {
  scoreson = on;
  menu::set(int(on)-1);
}

void resetmovement(dynent *d) {
  d->k_left = false;
  d->k_right = false;
  d->k_up = false;
  d->k_down = false;
  d->jumpnext = false;
  d->strafe = 0;
  d->move = 0;
}

// reset player state not persistent accross spawns
static void spawnstate(dynent *d) {
  resetmovement(d);
  d->vel = zero;
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
      if (mode()==12) {
        d->gunselect = GUN_FIST;
        return;
      }  // eihrul's secret "instafist" mode
      d->health = 256;
      if (m_tarena) {
        int gun1 = rnd(4)+1;
        baseammo(d->gunselect = gun1);
        for (;;) {
          int gun2 = rnd(4)+1;
          if (gun1!=gun2) {
            baseammo(gun2);
            break;
          }
        }
      } else if (m_arena)    // insta arena
        d->ammo[GUN_RIFLE] = 100;
      else {
        loopi(4) baseammo(i+1);
        d->gunselect = GUN_CG;
      }
      d->ammo[GUN_CG] /= 2;
    }
  } else
    d->ammo[GUN_SG] = 5;
}

dynent *newdynent() {
  dynent *d = (dynent*) MALLOC(sizeof(dynent));
  d->o = zero;
  d->ypr = vec3f(270.f,0.f,0.f);
  d->maxspeed = 22.f;
  d->outsidemap = false;
  d->inwater = false;
  d->radius = 0.5f;
  d->o.y = d->eyeheight = 1.8f;
  d->aboveeye = 0.2f;
  d->frags = 0;
  d->plag = 0;
  d->ping = 0;
  d->lastupdate = int(lastmillis());
  d->enemy = NULL;
  d->monsterstate = 0;
  d->name[0] = d->team[0] = 0;
  d->blocked = false;
  d->lifesequence = 0;
  d->state = CS_ALIVE;
  spawnstate(d);
  return d;
}

static void respawnself() {
  spawnplayer(player1);
  showscores(false);
}
CMD(showscores);

void arenacount(dynent *d, int &alive, int &dead, char *&lastteam, bool &oneteam) {
  if (d->state!=CS_DEAD) {
    alive++;
    if (lastteam && strcmp(lastteam, d->team.c_str()))
      oneteam = false;
    lastteam = d->team.c_str();
  } else
    dead++;
}

static int arenarespawnwait = 0;
static int arenadetectwait  = 0;

void arenarespawn() {
  if (arenarespawnwait) {
    if (arenarespawnwait<lastmillis()) {
      arenarespawnwait = 0;
      con::out("new round starting... fight!");
      respawnself();
    }
  } else if (arenadetectwait==0 || arenadetectwait<lastmillis()) {
    arenadetectwait = 0;
    int alive = 0, dead = 0;
    char *lastteam = NULL;
    bool oneteam = true;
    loopv(players) if (players[i])
      arenacount(players[i], alive, dead, lastteam, oneteam);
    arenacount(player1, alive, dead, lastteam, oneteam);
    if (dead>0 && (alive<=1 || (m_teammode && oneteam))) {
      con::out("arena round is over! next round in 5 seconds...");
      if (alive)
        con::out("team %s is last man standing", lastteam);
      else
        con::out("everyone died!");
      arenarespawnwait = int(lastmillis())+5000;
      arenadetectwait = int(lastmillis()) + 10000;
      player1->ypr.z = 0.f ;
    }
  }
}

void zapdynent(dynent *&d) {
  if (d) FREE(d);
  d = NULL;
}

static void otherplayers() {
  loopv(players) if (players[i]) {
    const int lagtime = int(lastmillis())-players[i]->lastupdate;
    if (lagtime>1000 && players[i]->state==CS_ALIVE) {
      players[i]->state = CS_LAGGED;
      continue;
    }
    // use physics to extrapolate player position
    if (lagtime && players[i]->state != CS_DEAD &&
      (!demo::playing() || i!=demo::clientnum()))
      physics::moveplayer(players[i], 2, false);
  }
}

static void respawn() {
  if (player1->state==CS_DEAD) {
    player1->attacking = false;
    if (m_arena) {
      con::out("waiting for new round to start...");
      return;
    }
    if (m_sp) {
      setnextmode(gamemode);
      client::changemap(clientmap.c_str());
      return;
    }
    // if we die in SP we try the same map again
    respawnself();
  }
}

static int sleepwait = 0;
static fixedstring sleepcmd;
static void sleepf(const char *msec, const char *cmd) {
  sleepwait = atoi(msec)+int(lastmillis());
  strcpy_s(sleepcmd, cmd);
}
CMDN(sleep, sleepf);

void updateworld(int millis) {
  if (lastmillis()) {
    setcurtime(millis - lastmillis());
    if (sleepwait && lastmillis()>sleepwait) {
      sleepwait = 0;
      script::execstring(sleepcmd.c_str());
    }
    physics::frame();
    checkquad(int(curtime()));
    if (m_arena)
      arenarespawn();
    moveprojectiles(float(curtime()));
    demo::playbackstep();
    if (!demo::playing()) {
      if (client::getclientnum()>=0)
        shoot(player1, worldpos_); // only shoot when connected to server
      // do this first, so we have most accurate information when our player
      // moves
      client::gets2c();
    }
    otherplayers();
    if (!demo::playing()) {
      monsterthink();
      if (player1->state==CS_DEAD) {
        if (lastmillis()-player1->lastaction<2000) {
          player1->move = player1->strafe = 0;
          physics::moveplayer(player1, 10, false);
        } else if (!m_arena && !m_sp && lastmillis()-player1->lastaction>10000)
          respawn();
      } else if (!intermission) {
        physics::moveplayer(player1, 20, true);
        checkitems();
      }
      // do this last, to reduce the effective frame lag
      client::c2sinfo(player1);
    }
  }
  setlastmillis(millis);
}

void entinmap(dynent *d) {
  loopi(100) { // try max 100 times
    float dx = (rnd(21)-10)/10.0f*i;  // increasing distance
    float dy = (rnd(21)-10)/10.0f*i;
    d->o.x += dx;
    d->o.y += dy;
    if (physics::collide(d, true)) return;
    d->o.x -= dx;
    d->o.y -= dy;
  }
  con::out("can't find entity spawn spot! (%d, %d)", int(d->o.x), int(d->o.y));
}

static int spawncycle = -1;
// TODO static int fixspawn = 2;

void spawnplayer(dynent *d) {
#if 0 // TODO
  const int r = fixspawn-->0 ? 4 : rnd(10)+1;
  loopi(r) spawncycle = world::findentity(PLAYERSTART, spawncycle+1);
  if (spawncycle!=-1) {
    d->o.x = ents[spawncycle].x;
    d->o.y = ents[spawncycle].y;
    d->o.z = 10.f;// TODO handle real height later
    d->yaw = ents[spawncycle].attr1;
    d->ypr.y = 0;
    d->ypr.z = 0;
  } else
#endif
  {
    d->o.x = d->o.y = 0.f;
    d->o.z = 4.f;
  }
  entinmap(d);
  spawnstate(d);
  d->state = CS_ALIVE;
}

#define DIRECTION(name,v,d,s,os) \
void name(bool isdown) { \
  player1->s = con::iskeydown(); \
  player1->v = con::iskeydown()? d : (player1->os ? -(d) : 0); \
  player1->lastmove = int(lastmillis()); \
}\
CMD(name);
DIRECTION(backward, move,   -1, k_down,  k_up);
DIRECTION(forward,  move,    1, k_up,    k_down);
DIRECTION(left,     strafe,  1, k_left,  k_right);
DIRECTION(right,    strafe, -1, k_right, k_left);
#undef DIRECTION

static void attack(bool on) {
  if (intermission)
    return;
  if (edit::mode())
    edit::editdrag(con::iskeydown());
  else if ((player1->attacking = con::iskeydown()) != 0)
    respawn();
}
CMD(attack);

static void jumpn(bool on) {
  if (!con::iskeydown()) return;
  if (!intermission && (player1->jumpnext = con::iskeydown())) respawn();
}
CMDN(jump, jumpn);

void fixplayer1range() {
  const float MAXPITCH = 90.0f;
  if (player1->ypr.y>MAXPITCH) player1->ypr.y = MAXPITCH;
  if (player1->ypr.y<-MAXPITCH) player1->ypr.y = -MAXPITCH;
  while (player1->ypr.x<0.0f) player1->ypr.x += 360.0f;
  while (player1->ypr.x>=360.0f) player1->ypr.x -= 360.0f;
}

void mousemove(int dx, int dy) {
  const float SENSF = 33.0f; // try match quake sens
  if (player1->state==CS_DEAD || intermission) return;
  player1->ypr.x += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
  player1->ypr.y -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
  fixplayer1range();
}

void selfdamage(int damage, int actor, dynent *act) {
  if (player1->state!=CS_ALIVE || edit::mode() || intermission)
    return;
  // TODO rr::damageblend(damage);
  demo::blend(damage);
  // let armour absorb when possible
  int ad = damage*(player1->armourtype+1)*20/100;
  if (ad>player1->armour)
    ad = player1->armour;
  player1->armour -= ad;
  damage -= ad;
  float droll = damage/0.5f;
  // give player a kick depending on amount of damage
  player1->ypr.z += player1->ypr.z>0 ? droll :
    (player1->ypr.z<0 ? -droll : (rnd(2) ? droll : -droll));
  if ((player1->health -= damage)<=0) {
    if (actor==-2)
      con::out("you got killed by %s!", act->name);
    else if (actor==-1) {
      actor = client::getclientnum();
      con::out("you suicided!");
      client::addmsg(1, 2, SV_FRAGS, --player1->frags);
    } else {
      dynent *a = getclient(actor);
      if (a) {
        if (isteam(a->team.c_str(), player1->team.c_str()))
          con::out("you got fragged by a teammate (%s)", a->name);
        else
          con::out("you got fragged by %s", a->name);
      }
    }
    showscores(true);
    client::addmsg(1, 2, SV_DIED, actor);
    player1->lifesequence++;
    player1->attacking = false;
    player1->state = CS_DEAD;
    player1->ypr.y = 0;
    player1->ypr.z = 60;
    sound::play(sound::DIE1+rnd(2));
    spawnstate(player1);
    player1->lastaction = int(lastmillis());
  } else
    sound::play(sound::PAIN6);
}

void timeupdate(int timeremain) {
  if (!timeremain) {
    intermission = true;
    player1->attacking = false;
    con::out("intermission:");
    con::out("game has ended!");
    showscores(true);
  } else
    con::out("time remaining: %d minutes", timeremain);
}

dynent *getclient(int cn) {
  if (cn<0 || cn>=MAXCLIENTS) {
    client::neterr("clientnum");
    return NULL;
  }
  while (cn>=players.length())
    players.add(NULL);
  return players[cn] ? players[cn] : (players[cn] = newdynent());
}

void initclient() {
  clientmap[0] = 0;
  client::initclientnet();
}

void startmap(const char *name) {
  if (client::netmapstart() && m_sp) {
    gamemode = 0;
    con::out("coop sp not supported yet");
  }
  sleepwait = 0;
  monsterclear();
  projreset();
  spawncycle = -1;
  spawnplayer(player1);
  player1->frags = 0;
  loopv(players) if (players[i]) players[i]->frags = 0;
  resetspawns();
  strcpy_s(clientmap, name);
  if (edit::mode()) edit::toggleedit();
  script::execstring("q.gamespeed=100");
  showscores(false);
  intermission = false;
  con::out("game mode is %s", modestr(mode()));
}

// render players & monsters: very messy ad-hoc handling of animation frames,
// should be made more configurable
static const int frame[] = {178, 184, 190, 137, 183, 189, 197, 164, 46, 51, 54, 32, 0,  0, 40, 1,  162, 162, 67, 168};
static const int range[] = {6,   6,   8,   28,  1,   1,   1,   1,   8,  19, 4,  18, 40, 1, 6,  15, 1,   1,   1,  1  };

void renderclient(dynent *d, bool team, const char *mdlname, bool hellpig, float scale) {
  int n = 3;
  auto speed = 100.0f;
  auto my = d->o.y-d->eyeheight+1.55f*scale;
  int cast = (int) (uintptr_t) d;
  int basetime = -((int(cast))&0xFFF);
  if (d->state==CS_DEAD) {
    int r;
    if (hellpig) {
      n = 2;
      r = range[3];
    } else {
      n = cast%3;
      r = range[n];
    }
    basetime = int(d->lastaction);
    int t = int(lastmillis())-d->lastaction;
    if (t<0 || t>20000) return;
    if (t>(r-1)*100) {
      n += 4; if (t>(r+10)*100) {
        t -= (r+10)*100;
        my -= t*t/10000000000.0f*t;
      }
    }
    if (my<-1000) return;
  }
  else if (d->state==CS_EDITING) n = 16;
  else if (d->state==CS_LAGGED) n = 17;
  else if (d->monsterstate==M_ATTACKING) n = 8;
  else if (d->monsterstate==M_PAIN) n = 10;
  else if ((!d->move && !d->strafe) || !d->moving) n = 12;
  else if (!d->onfloor && d->timeinair>100) n = 18;
  else {
    n = 14;
    speed = 1200/d->maxspeed*scale;
    if (hellpig) speed = 300/d->maxspeed;
  }
  if (hellpig) {
    n++;
    scale *= 32;
    my -= 1.9f;
  }

  const auto norxfm = mat3x3f::rotate(d->ypr.x, vec3f(0.f,-1.f,0.f))
                    * mat3x3f::rotate(d->ypr.y, vec3f(-1.f,0.f,0.f));
  const auto posxfm = ogl::matrix(ogl::MODELVIEW)
                    * mat4x4f::translate(vec3f(d->o.x, my, d->o.z))
                    * mat4x4f(norxfm);
  md2::render(mdlname, frame[n], range[n], posxfm, norxfm, team, scale, speed, 0, float(basetime));
}

void renderclients() {
  dynent *d;
  loopv(players)
    if ((d = players[i]) && (!demo::playing() || i!=demo::clientnum()))
      renderclient(d, isteam(player1->team.c_str(), d->team.c_str()),
                   "monster/ogro", false, 1.f);
}

struct sline { fixedstring s; };
static vector<sline> scorelines;

static void renderscore(dynent *d) {
  fixedstring lag(fmt, "%d", d->plag);
  fixedstring name(fmt, "(%s)", d->name);
  scorelines.add().s.fmt("%d\t%s\t%d\t%s\t%s",
    d->frags, d->state==CS_LAGGED ? "LAG" : lag, d->ping,
    d->team.c_str(), d->state==CS_DEAD ? name : d->name.c_str());
  menu::manual(0, scorelines.length()-1, scorelines.last().s.c_str());
}

static const int maxteams = 4;
static char *teamname[maxteams];
static int teamscore[maxteams], teamsused;
static fixedstring teamscores;

static void addteamscore(dynent *d) {
  if (!d) return;
  loopi(teamsused) if (strcmp(teamname[i], d->team.c_str())==0) {
    teamscore[i] += d->frags;
    return;
  }
  if (teamsused==maxteams) return;
  teamname[teamsused] = d->team.c_str();
  teamscore[teamsused++] = d->frags;
}

void renderscores() {
  if (!scoreson) return;
  scorelines.setsize(0);
  if (!demo::playing()) renderscore(player1);
  loopv(players) if (players[i])
    renderscore(players[i]);
  menu::sort(0, scorelines.length());
  if (m_teammode) {
    teamsused = 0;
    loopv(players)
      addteamscore(players[i]);
    if (!demo::playing()) addteamscore(player1);
    teamscores[0] = 0;
    loopj(teamsused) {
      fixedstring sc(fmt, "[ %s: %d ]", teamname[j], teamscore[j]);
      strcat_s(teamscores, sc.c_str());
    }
    menu::manual(0, scorelines.length(), "");
    menu::manual(0, scorelines.length()+1, teamscores.c_str());
  }
}

void clean() {
  cleanentities();
  cleanmonsters();
}
} /* namespace game */
} /* namespace q */
#endif // !defined(STANDALONE)

namespace q {
namespace game {
static const char *modenames[] = {
  "SP", "DMSP", "ffa/default", "coopedit", "ffa/duel", "teamplay",
  "instagib", "instagib team", "efficiency", "efficiency team",
  "insta arena", "insta clan arena", "tactics arena", "tactics clan arena",
};
const char *modestr(int n) {return (n>=-2 && n<12) ? modenames[n+2] : "unknown";}
} /* namespace game */
} /* namespace q */

