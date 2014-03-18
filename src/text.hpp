/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "math.hpp"

namespace q {
namespace text {
void start();
void finish();
vec2f fontdim();
vec2f displaydim();
u32 oglfont();
void resetdefaultwidth();
void displaywidth(float w);
void thickness(float t);
void outlinecolor(const vec4f &c);
void outlinewidth(float w);
float fontratio(); // height/width
float width(const char *str);
void drawf(const char *fstr, vec2f pos, ...);
void draw(const char *str, vec2f pos);
void drawf(const char *fstr, float x, float y, ...);
INLINE void draw(const char *str, float x, float y) {
  draw(str, vec2f(x,y));
}
} /* namespace text */
} /* namespace q */

