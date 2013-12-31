/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.hpp -> exposes rendering routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace rr {

static const int VIRTW = 2400; // screen width for text & HUD
static const int VIRTH = 1800; // screen height for text & HUD
static const int FONTH = 64; // font height
static const int PIXELTAB = VIRTW / 12; // tabulation size in pixels

void start();
void finish();
void hud(int w, int h, int curfps);
void frame(int w, int h, int curfps);

} /* namespace rr */
} /* namespace q */

