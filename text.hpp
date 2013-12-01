/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace text {
int width(const char *str);
void drawf(const char *fstr, int left, int top, int gl_num, ...);
void draw(const char *str, int left, int top, int gl_num);
void drawenvbox(int t, int w);
} /* namespace text */
} /* namespace q */

