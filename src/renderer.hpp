/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.hpp -> exposes rendering routines
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"

namespace q {
namespace rr {
static const float VIRTW = 1024.f; // for scalable UI
static const float PIXELTAB = 1.f; // tabulation size
extern float VIRTH;                // depends on aspect ratio

// particle types
enum {
  PT_SPARKS = 0,
  PT_SMALL_SMOKE = 1,
  PT_BLUE_ENTITY = 2,
  PT_BLOOD_SPATS = 3,
  PT_YELLOW_FIREBALL = 4,
  PT_GREY_SMOKE = 5,
  PT_BLUE_FIREBALL = 6,
  PT_GREEN_FIREBALL = 7,
  PT_RED_SMOKE = 8
};

// start and end renderer
void start();
void finish();

// simple primitives
void line(int x1, int y1, float z1, int x2, int y2, float z2);
void box(const vec3i &start, const vec3i &size, const vec3f &col);
void dot(int x, int y, float z);
void blendbox(float x1, float y1, float x2, float y2, bool border);

// particle rendering (actually append particles to render)
void particle_splash(int type, int num, int fade, const vec3f &p);
void particle_trail(int type, int fade, const vec3f &s, const vec3f &e);

void hud(int w, int h, int curfps);
void frame(int w, int h, int curfps);
vec2f scrdim();
} /* namespace rr */
} /* namespace q */

