/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - editing.hpp -> implements editing routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace edit {

IVAR(editing,0,0,1);
static bool editmode = false;
bool mode(void) { return editmode; }

void toggleedit(void) {
  if (game::player1->state==CS_DEAD) return; // do not allow dead players to edit to avoid state confusion
  if (!editmode && !client::allowedittoggle()) return; // not in most client::multiplayer modes
#if 0
  if (!(editmode = !editmode))
    game::entinmap(game::player1); // find spawn closest to current floating pos
  else {
#else
  if ((editmode = !editmode)) {
#endif
    game::player1->health = 100;
    if (m_classicsp) game::monsterclear(); // all monsters back at their spawns for editing
    game::projreset();
  }
  sys::keyrepeat(editmode);
  // selset = false;
  editing = editmode;
}
CMDN(edittoggle, toggleedit, "");

void pruneundos(int maxremain) {}

// two mode of editions: extrusion of cubes / displacement of corners
IVARF(editcorner, 0, 0, 1, con::out("edit mode is 'edit%s", editcorner?"corner'":"cube'"));

bool noteditmode(void) {
  if (!editmode)
    con::out("this function is only allowed in edit mode");
  return !editmode;
}

static bool noselection(void) { return true; }
void editdrag(bool) {}
void cursorupdate(void) {} // called every frame from hud

} /* namespace edit */
} /* namespace q */

