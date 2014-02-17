/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.hpp -> exposes rendering routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"

namespace q {
namespace rr {

static const float VIRTW = 1024.f; // for scalable UI
static const float PIXELTAB = 1.f; // tabulation size
extern float VIRTH;                // depends on aspect ratio

void start();
void finish();
void hud(int w, int h, int curfps);
void frame(int w, int h, int curfps);
vec2f scrdim();

} /* namespace rr */
} /* namespace q */

