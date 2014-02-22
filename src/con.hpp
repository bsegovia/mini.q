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
float height();
void keypress(int code, bool isdown, int cooked);
bool active();
void render();
} /* namespace con */
} /* namespace q */

