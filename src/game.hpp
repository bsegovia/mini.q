/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - game.hpp -> exposes game routines and variables
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"
#include "base/sys.hpp"

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

// player matrices computed at the beginning of the frame
extern mat4x4f mvmat, pmat, mvpmat, invmvpmat, dirinvmvpmat;

#define m_noitems     (game::mode()>=4)
#define m_noitemsrail (game::mode()<=5)
#define m_arena       (game::mode()>=8)
#define m_tarena      (game::mode()>=10)
#define m_teammode    (game::mode()&1 && game::mode()>2)
#define m_sp          (game::mode()<0)
#define m_dmsp        (game::mode()==-1)
#define m_classicsp   (game::mode()==-2)
#define isteam(a,b)   (m_teammode && strcmp(a, b)==0)

GLOBAL_VAR_DECL(mode,int);
GLOBAL_VAR_DECL(nextmode,int);
GLOBAL_VAR_DECL(worldpos,const vec3f&);
GLOBAL_VAR_DECL(lastmillis,double);
GLOBAL_VAR_DECL(curtime,double);
void mousemove(int dx, int dy); 
void updateworld(int millis);
void initclient(void);
void spawnplayer(dynent *d);
void selfdamage(int damage, int actor, dynent *act);
dynent *newdynent(void);
const char *getclientmap(void);
const char *modestr(int n);
void zapdynent(dynent *&d);
dynent *getclient(int cn);
void timeupdate(int timeremain);
void resetmovement(dynent *d);
void fixplayer1range(void);
void entinmap(dynent *d);
void startmap(const char *name);
void renderclients(void);
void renderclient(dynent *d, bool team, const char *name, bool hellpig, float scale);
void renderscores(void);
void cleanentities(void);
void cleanmonsters(void);
void clean(void);
void selfdamage(int damage, int actor, dynent *act);
void setmatrices(float fovy, float farplane, float w, float h);
} /* namespace game */
} /* namespace q */

