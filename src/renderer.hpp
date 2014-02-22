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

// simple primitives
void line(int x1, int y1, float z1, int x2, int y2, float z2);
void box(const vec3i &start, const vec3i &size, const vec3f &col);
void dot(int x, int y, float z);
void blendbox(float x1, float y1, float x2, float y2, bool border);

void start();
void finish();
void hud(int w, int h, int curfps);
void frame(int w, int h, int curfps);
vec2f scrdim();
} /* namespace rr */
} /* namespace q */

