/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - monster.hpp -> implements monster AI
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace game {

static dvector monsters;
static int nextmonster, spawnremain, numkilled, monstertotal, mtimestart;

VARF(skill, 1, 3, 10, con::out("skill is now %d", skill));

dvector &getmonsters(void) { return monsters; };
void restoremonsterstate(void) { // for savegames
  loopv(monsters)
  if (monsters[i]->state==CS_DEAD)
    numkilled++;
}

static const int TOTMFREQ = 13;
static const int NUMMONSTERTYPES = 8;

struct monstertype { // see docs for how these values modify behaviour
  short gun, speed, health, freq, lag, rate, pain, loyalty, mscale, bscale;
  short painsound, diesound;
  const char *name, *mdlname;
} static const monstertypes[NUMMONSTERTYPES] = {
  {GUN_FIREBALL,  15, 100, 3, 0,   100, 800, 1, 10, 10, sound::PAINO, sound::DIE1,   "an ogre",     "monster/ogro"   },
  {GUN_CG,        18,  70, 2, 70,   10, 400, 2,  8,  9, sound::PAINR, sound::DEATHR, "a rhino",     "monster/rhino"  },
  {GUN_SG,        14, 120, 1, 100, 300, 400, 4, 14, 14, sound::PAINE, sound::DEATHE, "ratamahatta", "monster/rat"    },
  {GUN_RIFLE,     15, 200, 1, 80,  300, 300, 4, 18, 18, sound::PAINS, sound::DEATHS, "a slith",     "monster/slith"  },
  {GUN_RL,        13, 500, 1, 0,   100, 200, 6, 24, 24, sound::PAINB, sound::DEATHB, "bauul",       "monster/bauul"  },
  {GUN_BITE,      22,  50, 3, 0,   100, 400, 1, 12, 15, sound::PAINP, sound::PIGGR2, "a hellpig",   "monster/hellpig"},
  {GUN_ICEBALL,   12, 250, 1, 0,    10, 400, 6, 18, 18, sound::PAINH, sound::DEATHH, "a knight",    "monster/knight" },
  {GUN_SLIMEBALL, 15, 100, 1, 0,   200, 400, 2, 13, 10, sound::PAIND, sound::DEATHD, "a goblin",    "monster/goblin" },
};

dynent *basicmonster(int type, int yaw, int state, int trigger, int move) {
  if (type>=NUMMONSTERTYPES) {
    con::out("warning: unknown monster in spawn: %d", type);
    type = 0;
  }
  auto m = newdynent();
  const monstertype *t = &monstertypes[m->mtype = type];
  m->eyeheight = 2.0f;
  m->aboveeye = 1.9f;
  m->radius *= t->bscale/10.0f;
  m->eyeheight *= t->bscale/10.0f;
  m->aboveeye *= t->bscale/10.0f;
  m->monsterstate = state;
  if (state!=M_SLEEP) spawnplayer(m);
  m->trigger = int(lastmillis())+trigger;
  m->targetyaw = m->ypr.x = float(yaw);
  m->move = move;
  m->enemy = player1;
  m->gunselect = t->gun;
  m->maxspeed = float(t->speed);
  m->health = t->health;
  m->armour = 0;
  loopi(NUMGUNS) m->ammo[i] = 8192;
  m->ypr.y = 0;
  m->ypr.z = 0;
  m->state = CS_ALIVE;
  m->anger = 0;
  strcpy_s(m->name, t->name);
  monsters.add(m);
  return m;
}

// spawn a random monster according to freq distribution in DMSP
static void spawnmonster(void) {
  int n = rnd(TOTMFREQ), type;
  for (int i = 0; ; i++)
    if ((n -= monstertypes[i].freq)<0) {
      type = i;
      break;
    }
  basicmonster(type, rnd(360), M_SEARCH, 1000, 1);
}

void cleanmonsters() {
  loopv(monsters) FREE(monsters[i]);
  monsters.destroy();
  numkilled = 0;
  monstertotal = 0;
  spawnremain = 0;
}

// called after map start of when toggling edit mode to reset/spawn all monsters
// to initial state
void monsterclear(void) {
  cleanmonsters();
  if (m_dmsp) {
    nextmonster = mtimestart = int(lastmillis())+1000;
    monstertotal = spawnremain = 1; // XXX mode()<0 ? skill*10 : 0;
  } else if (m_classicsp) {
    mtimestart = int(lastmillis());
    loopv(ents) if (ents[i].type==MONSTER) {
      auto m = basicmonster(ents[i].attr2, ents[i].attr1, M_SLEEP, 100, 0);
      m->o.x = ents[i].x;
      m->o.y = ents[i].y;
      m->o.z = ents[i].z;
      entinmap(m);
      monstertotal++;
    }
  }
}

bool enemylos(dynent *m, vec3f &v) {
  // no occlusion, return the target itself
  v = m->enemy->o;
  return true;
}

// monster AI is sequenced using transitions: they are in a particular state
// where they execute a particular behaviour until the trigger time is hit, and
// then they reevaluate their situation based on the current state, the
// environment etc., and transition to the next state. Transition timeframes are
// parametrized by difficulty level (skill), faster transitions means quicker
// decision making means tougher AI.
// n = at skill 0, n/2 = at skill 10, r = added random factor
void transition(dynent *m, int state, int moving, int n, int r) {
  m->monsterstate = state;
  m->move = moving;
  n = n*130/100;
  m->trigger = int(lastmillis())+n-skill*(n/16)+rnd(r+1);
}

void normalise(dynent *m, float angle) {
  while (m->ypr.x<angle-180.0f) m->ypr.x += 360.0f;
  while (m->ypr.x>angle+180.0f) m->ypr.x -= 360.0f;
}

// main AI thinking routine, called every frame for every monster
void monsteraction(dynent *m) {
  if (m->enemy->state==CS_DEAD) {
    m->enemy = player1;
    m->anger = 0;
  }
  normalise(m, m->targetyaw);
  if (m->targetyaw>m->ypr.x) { // slowly turn monster towards his target
    m->ypr.x += float(curtime())*0.5f;
    if (m->targetyaw<m->ypr.x) m->ypr.x = m->targetyaw;
  } else {
    m->ypr.x -= float(curtime())*0.5f;
    if (m->targetyaw>m->ypr.x) m->ypr.x = m->targetyaw;
  }

  const float disttoenemy = distance(m->o, m->enemy->o);
  m->ypr.y = atan2(m->enemy->o.y-m->o.y, disttoenemy)*180.f/float(pi);

  // special case: if we run into scenery
  if (m->blocked) {
    m->blocked = false;
    // try to jump over obstackle (rare)
    if (!rnd(20000/monstertypes[m->mtype].speed))
      m->jumpnext = true;
    // search for a way around (common)
    else if (m->trigger<lastmillis() && (m->monsterstate!=M_HOME || !rnd(5))) {
      m->targetyaw += 180.f+rnd(180); // patented "random walk" AI pathfinding (tm) ;)
      transition(m, M_SEARCH, 1, 400, 1000);
    }
  }

  const auto enemyyaw = -atan2(m->enemy->o.x-m->o.x, m->enemy->o.z-m->o.z)/float(pi)*180.f+180.f;

  switch (m->monsterstate) {
    case M_PAIN:
    case M_ATTACKING:
    case M_SEARCH:
      if (m->trigger<lastmillis()) transition(m, M_HOME, 1, 100, 200);
    break;
    // state classic sp monster start in, wait for visual contact
    case M_SLEEP: {
      vec3f target;
      // skip running physics
      if (edit::mode() || !enemylos(m, target)) return;
      normalise(m, enemyyaw);
      const auto angle = abs(enemyyaw-m->ypr.x);
      if (disttoenemy<8.f ||  // the better the angle to the player
         (disttoenemy<16.f && angle<135.f) || // the further the monster can
         (disttoenemy<32.f && angle<90.f) ||  // see/hear
         (disttoenemy<64.f && angle<45.f) ||
         angle<10) {
        transition(m, M_HOME, 1, 500, 200);
        sound::play(sound::GRUNT1+rnd(2), &m->o);
      }
    }
    break;

    // this state is the delay between wanting to shoot and actually firing
    case M_AIMING:
      if (m->trigger<lastmillis()) {
        m->lastaction = 0;
        m->attacking = true;
        shoot(m, m->attacktarget);
        transition(m, M_ATTACKING, 0, 600, 0);
      }
    break;

    // monster has visual contact, heads straight for player and may want to
    // shoot at any time
    case M_HOME:
      m->targetyaw = enemyyaw;
      if (m->trigger<lastmillis()) {
        vec3f target;
        // no visual contact anymore, let monster get as close as possible then
        // search for player
        if (!enemylos(m, target))
          transition(m, M_HOME, 1, 800, 500);
        else  { // the closer the monster is the more likely he wants to shoot
          if (!rnd((int)disttoenemy/3+1) && m->enemy->state==CS_ALIVE) { // get ready to fire
            m->attacktarget = target;
            transition(m, M_AIMING, 0, monstertypes[m->mtype].lag, 10);
          } else // track player some more
            transition(m, M_HOME, 1, monstertypes[m->mtype].rate, 0);
        }
      }
    break;
  }
  physics::moveplayer(m, 1, false); // use physics to move monster
}

void monsterpain(dynent *m, int damage, dynent *d) {
  if (d->monsterstate) { // a monster hit us
    if (m!=d) { // guard for RL guys shooting themselves :)
      m->anger++; // don't attack straight away, first get angry
      int anger = m->mtype==d->mtype ? m->anger/2 : m->anger;
      if (anger>=monstertypes[m->mtype].loyalty)
        m->enemy = d; // monster infight if very angry
    }
  } else { // player hit us
    m->anger = 0;
    m->enemy = d;
  }
  transition(m, M_PAIN, 0, monstertypes[m->mtype].pain,200);      // in this state monster won't attack
  if ((m->health -= damage)<=0) {
    m->state = CS_DEAD;
    m->lastaction = int(lastmillis());
    player1->frags = ++numkilled;
    sound::play(monstertypes[m->mtype].diesound, &m->o);
    int remain = monstertotal-numkilled;
    if (remain>0 && remain<=5) con::out("only %d monster(s) remaining", remain);
  } else
    sound::play(monstertypes[m->mtype].painsound, &m->o);
}

void endsp(bool allkilled) {
  con::out(allkilled ? "you have cleared the map!" : "you reached the exit!");
  con::out("score: %d kills in %d seconds", numkilled, (lastmillis()-mtimestart)/1000);
  monstertotal = 0;
  server::startintermission();
}

void monsterthink(void) {
  if (m_dmsp && spawnremain && lastmillis()>nextmonster) {
    if (spawnremain--==monstertotal)
      con::out("The invasion has begun!");
    nextmonster = int(lastmillis())+1000;
    spawnmonster();
  }

  if (monstertotal && !spawnremain && numkilled==monstertotal)
    endsp(true);

  // equivalent of player entity touch, but only teleports are used
  loopv(ents) {
    entity &e = ents[i];
    if (e.type!=TELEPORT) continue;
    vec3f v(float(e.x), float(e.y), 0.f);
    loopv(monsters) if (monsters[i]->state==CS_DEAD) {
      if (lastmillis()-monsters[i]->lastaction<2000) {
        monsters[i]->move = 0;
        physics::moveplayer(monsters[i], 1, false);
      }
    } else {
      v.z += monsters[i]->eyeheight;
      const float dist = distance(monsters[i]->o, v);
      v.z -= monsters[i]->eyeheight;
      if (dist<4) game::teleport((int)(&e-&ents[0]), monsters[i]);
    }
  }
  loopv(monsters) if (monsters[i]->state==CS_ALIVE)
    monsteraction(monsters[i]);
}

void rendermonsters() {
  loopv(monsters)
    renderclient(monsters[i], false, monstertypes[monsters[i]->mtype].mdlname,
      monsters[i]->mtype==5, monstertypes[monsters[i]->mtype].mscale/10.0f);
}
} /* namespace game */
} /* namespace q */

