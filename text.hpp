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
u32 getoglfont();
void loadfontshader(bool fatalerr, bool fromfile);
void charwidth(float w);
void thickness(float t);
void outlinecolor(const vec4f &c);
void outlinewidth(float w);
float width(const char *str);
void drawf(const char *fstr, const vec2f &pos, ...);
void draw(const char *str, const vec2f &pos);
} /* namespace text */
} /* namespace q */

