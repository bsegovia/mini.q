/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.hpp -> exposes game routines and variables
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"
#include "stl.hpp"

namespace q {
namespace game {

// current game mode
extern int gamemode;

// game modes
enum {
  MODE_DMSP=-1,
  MODE_CLASSICSP=-2,
  FFA_DEFAULT=0,
  COOP_EDIT=1,
  FFA_DUEL=2,
  TEAMPLAY=3,
  INSTAGIB=4,
  INSTAGIB_TEAM=5,
  EFFICIENCY=6,
  EFFICIENCY_TEAM=7,
  INSTA_ARENA=8,
  INSTA_CLAN_ARENA=9,
  TACTICS_ARENA=10,
  TACTICS_CLAN_ARENA=11
};

// game mode specific helper macros
#define m_noitems     (gamemode>=4)
#define m_noitemsrail (gamemode<=5)
#define m_arena       (gamemode>=8)
#define m_tarena      (gamemode>=10)
#define m_teammode    (gamemode&1 && gamemode>2)
#define m_sp          (gamemode<0)
#define m_dmsp        (gamemode==-1)
#define m_classicsp   (gamemode==-2)
#define isteam(a,b)   (m_teammode && strcmp(a, b)==0)

// all gun/weapon types
enum {
  GUN_FIST = 0,
  GUN_SG,
  GUN_CG,
  GUN_RL,
  GUN_RIFLE,
  GUN_FIREBALL,
  GUN_ICEBALL,
  GUN_SLIMEBALL,
  GUN_BITE,
  NUMGUNS
};

// players and monsters
struct dynent {
  dynent();
  vec3f o, vel;           // origin, velocity
  vec3f ypr;              // yaw, pitch, roll
  int lastattackgun;      // last gun used for attack
  float lastaction;       // time of the last action
  float lastmove;         // time of last move
  float lastupdate;       // time of last update
  float maxspeed;         // meters per second: 8 for player
  float radius;           // radius of the entity
  float eyeheight;        // height of the eye
  float aboveeye;         // size of what is below the eye
  float timeinair;        // time spent in air from jump to jump
  int move:2, strafe:2;   // bwd, fwd, left, right
  uint kleft:1, kright:1; // see input code
  uint kup:1, kdown:1;    // see input code
  uint flycam:1;          // no collision / free camera
  uint gun:6;             // gun selected by the entity
  bool onfloor;           // true if contact with floor
  bool jump;              // need to apply jump related physics code
  int lifesequence;       // sequence id for each respawn, used in damage test
  int state:4;            // one of CS_* below
  int armourtype:3;       // one of A_* below
  int frags;              // total number of frags done by the entity
  int health:16;          // remaining life points
  int armour:16;          // remaining armor points
  int quadmillis;         // remaining ms before quad ends up for entity
  int gunselect;          // gun currently used to shoot
  int gunwait;            // delay in ms before being able to shoot again
  bool attacking;         // is it currently attacking?
  int ammo[NUMGUNS];      // ammo for each gun
  int monsterstate:8;     // one of M_*, M_NONE means human
  int mtype:8;            // see monster.cpp
  dynent *enemy;          // monster wants to kill this entity
  float targetyaw;        // monster wants to look in this direction
  bool blocked, moving;   // used by physics to signal ai
  int trigger;            // millis at which transition to another monsterstate takes place
  vec3f attacktarget;     // delayed attacks
  int anger;              // how many times already hit by fellow monster
  string name;            // name of entity
  string team;            // team it belongs to
};

// state of the entity
enum {CS_ALIVE = 0, CS_DEAD, CS_LAGGED, CS_EDITING};

// static entity types
enum {
  NOTUSED = 0,                // entity slot not in use in map
  LIGHT,                      // lightsource, attr1 = radius, attr2 = intensity
  PLAYERSTART,                // attr1 = angle
  I_SHELLS, I_BULLETS, I_ROCKETS, I_ROUNDS,
  I_HEALTH, I_BOOST,
  I_GREENARMOUR, I_YELLOWARMOUR,
  I_QUAD,
  TELEPORT,                   // attr1 = idx
  TELEDEST,                   // attr1 = angle, attr2 = idx
  MAPMODEL,                   // attr1 = angle, attr2 = idx
  MONSTER,                    // attr1 = angle, attr2 = monstertype
  CARROT,                     // attr1 = tag, attr2 = type
  JUMPPAD,                    // attr1 = zpush, attr2 = ypush, attr3 = xpush
  MAXENTTYPES
};

// map entity
struct persistent_entity {
  s16 x, y, z;  // cube aligned position
  s16 attr1;
  u8 type;      // type is one of the above
  u8 attr2, attr3, attr4;
};
struct entity : persistent_entity {bool spawned;};

// armour types... take 20/40/60 % off
enum {A_BLUE, A_GREEN, A_YELLOW};

struct mapmodelinfo { int rad, h, zoff, snap; const char *name; };

INLINE aabb getaabb(const dynent &d) {
  const auto fmin = d.o-vec3f(d.radius, d.eyeheight, d.radius);
  const auto fmax = d.o+vec3f(d.radius, d.aboveeye, d.radius);
  return aabb(fmin, fmax);
}

typedef vector<dynent*> dvector;
extern vector<entity> ents;        // map entities
extern dynent player;
extern dvector players;            // all the other clients (in multiplayer)
extern float lastmillis, curtime, speed;
void entinmap(dynent*);
void spawnplayer(dynent*);
void selfdamage(int damage, int actor, dynent*);
void mousemove(int dx, int dy);
void updateworld(float millis);
} /* namespace game */
} /* namespace q */

