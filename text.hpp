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
void charwidth(u32 w);
void thickness(float t);
void outlinecolor(const vec4f &c);
void outlinewidth(float w);
int width(const char *str);
void drawf(const char *fstr, int left, int top, ...);
void draw(const char *str, int left, int top);
void drawenvbox(int t, int w);
} /* namespace text */
} /* namespace q */

