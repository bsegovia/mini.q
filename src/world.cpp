/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - world.cpp -> implements routines to save/load maps
 -------------------------------------------------------------------------*/
#include "world.hpp"
#include "game.hpp"

namespace q {
namespace world {
void load(const char *mname) {
  game::startmap(mname);
}
} /* namespace world */
} /* namespace q */

