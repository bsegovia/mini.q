/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - editing.hpp -> exposes editing routines
 -------------------------------------------------------------------------*/
#pragma once

namespace q {
namespace edit {

bool mode(void);
void cursorupdate(void);
void toggleedit(void);
void editdrag(bool isdown);
bool noteditmode(void);
void pruneundos(int maxremain = 0);

} /* namespace edit */
} /* namespace q */

