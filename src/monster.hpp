/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - monster.hpp -> exposes monster AI
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"

namespace q {
namespace game {

enum {M_NONE, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING};
void monsterclear(void);
void restoremonsterstate(void);
void monsterthink(void);
void monsterrender(void);
dvector &getmonsters(void);
void monsterpain(dynent *m, int damage, dynent *d);
void endsp(bool allkilled);

} /* namespace game */
} /* namespace q */

