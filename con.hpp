/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - con.hpp -> exposes console functionalities
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace con {
void out(const char *s, ...);
void keypress(int code, bool isdown, int cooked);
} /* namespace con */
} /* namespace q */

