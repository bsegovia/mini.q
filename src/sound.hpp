#pragma once
#include "math.hpp"

namespace q {
namespace sound {

// Hardcoded sounds, defined in sounds.cfg
enum {
  JUMP = 0, LAND, RIFLE, PUNCH1, SG, CG,
  RLFIRE, RLHIT, WEAPLOAD, ITEMAMMO, ITEMHEALTH,
  ITEMARMOUR, ITEMPUP, ITEMSPAWN, TELEPORT, NOAMMO, PUPOUT,
  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5, PAIN6,
  DIE1, DIE2,
  FLAUNCH, FEXPLODE,
  SPLASH1, SPLASH2,
  GRUNT1, GRUNT2, RUMBLE,
  PAINO,
  PAINR, DEATHR,
  PAINE, DEATHE,
  PAINS, DEATHS,
  PAINB, DEATHB,
  PAINP, PIGGR2,
  PAINH, DEATHH,
  PAIND, DEATHD,
  PIGR1, ICEBALL, SLIMEBALL,
  JUMPPAD,
};

// init the sound module
void start(void);
// stop the sound module
void finish(void);
// play sound n at given location
void play(int n, const vec3f *loc = NULL);
// play sound n and send message to the server
void playc(int n);
// update the overall volume
void updatevol(void);
} /* namespace sound */
} /* namespace q */

