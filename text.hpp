/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"

namespace q {
namespace text {
u32 buildfont();
int width(const char *str);
void drawf(const char *fstr, int left, int top, ...);
void draw(const char *str, int left, int top);
void drawenvbox(int t, int w);
} /* namespace text */
} /* namespace q */

