/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.hpp -> exposes game routines and variables
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"
#include "sys.hpp"

namespace q {
namespace game {

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

#define m_noitems     (game::mode()>=4)
#define m_noitemsrail (game::mode()<=5)
#define m_arena       (game::mode()>=8)
#define m_tarena      (game::mode()>=10)
#define m_teammode    (game::mode()&1 && game::mode()>2)
#define m_sp          (game::mode()<0)
#define m_dmsp        (game::mode()==-1)
#define m_classicsp   (game::mode()==-2)
#define isteam(a,b)   (m_teammode && strcmp(a, b)==0)

// get / set current game mode
GLOBAL_VAR_DECL(mode,int);
// get / set the next game mode to be set (changed when map changes)
GLOBAL_VAR_DECL(nextmode,int);
// get / set current target of the crosshair in the world
GLOBAL_VAR_DECL(worldpos,const vec3f&);
// get / set last time
GLOBAL_VAR_DECL(lastmillis,double);
// get / set current frame time
GLOBAL_VAR_DECL(curtime,double);
// apply mouse movement to player
void mousemove(int dx, int dy); 
// main game update loop
void updateworld(int millis);
// create a new client
void initclient(void);
// place at random spawn. also used by monsters
void spawnplayer(dynent *d);
// damage arriving from the network, monsters, yourself, ends up here
void selfdamage(int damage, int actor, dynent *act);
// create a new blank player or monster
dynent *newdynent(void);
// get the map currently used by the client
const char *getclientmap(void);
// return the name of the mode currently played
const char *modestr(int n);
// free the entity
void zapdynent(dynent *&d);
// get entity for drawarray cn
dynent *getclient(int cn);
// useful for timi limited modes
void timeupdate(int timeremain);
// set the entity as not moving anymore
void resetmovement(dynent *d);
// properly clamp angle values (raw,pitch,yaw)
void fixplayer1range(void);
// brute force but effective way to find a free spawn spot in the map
void entinmap(dynent *d);
// called just after a map load
void startmap(const char *name);
// render all the clients
void renderclients(void);
// render the client
void renderclient(dynent *d, bool team, const char *name, bool hellpig, float scale);
// render the score on screen
void renderscores(void);
// release memory used by all entities
void cleanentities(void);
// release memory used by all monsters
void cleanmonsters(void);
// releas all memory used by the game system
void clean(void);

} /* namespace game */
} /* namespace q */

