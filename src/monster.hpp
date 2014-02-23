/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - monster.hpp -> exposes monster AI
 -------------------------------------------------------------------------*/
#pragma once
#include "entities.hpp"

namespace q {
namespace game {

enum {M_NONE, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING};
void cleanmonsters();
void monsterclear();
void restoremonsterstate();
void monsterthink();
void rendermonsters();
dvector &getmonsters();
void monsterpain(dynent *m, int damage, dynent *d);
void endsp(bool allkilled);
void finish();

} /* namespace game */
} /* namespace q */

