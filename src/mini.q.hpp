/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.hpp -> includes every game header
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
extern int fov, farplane, gamespeed, minmillis;
void start(int argc, char *argv[]);
void finish();
void swap();
} /* namespace q */

