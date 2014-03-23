/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - demo.hpp -> demo record/play back routines
 -------------------------------------------------------------------------*/
#pragma once
#include "base/sys.hpp"

// bump if dynent/netprotocol changes or any other savegame/demo data
#define SAVEGAMEVERSION 4

namespace q {
namespace demo {

void loadgamerest(void);
void incomingdata(u8 *buf, int len, bool extras = false);
void playbackstep(void);
void stop(void);
void stopifrecording(void);
void damage(int damage, vec3f &o);
void blend(int damage);
bool playing(void);
int clientnum(void);

} /* namespace demo */
} /* namespace q */

