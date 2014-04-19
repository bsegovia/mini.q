/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - menu.hpp -> exposes in-game menus
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace menu {
bool render(void);
void set(int menu);
void manual(int m, int n, const char *text);
void sort(int start, int num);
bool key(int code, bool isdown);
void newm(const char *name);
void finish();
} /* namespace menu */
} /* namespace q */

