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
void setkeydownflag(bool on);
bool iskeydown();
float height();
void keypress(int code, bool isdown);
void processtextinput(const char *txt);
bool active();
void render();
} /* namespace con */
} /* namespace q */

