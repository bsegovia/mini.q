/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - con.hpp -> exposes console functionalities
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace con {
void finish();
const char *curcmd();
void out(const char *s, ...);
void keypress(int code, int isdown /* 0 or 1 */, int cooked);
bool active();
void render();
} /* namespace con */
} /* namespace q */

