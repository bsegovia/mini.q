/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - csgdecl.hxx -> template to declare various csg evaluation routines
 -------------------------------------------------------------------------*/
// many points evaluation
void dist(const node *RESTRICT, const array3f &RESTRICT,
          const arrayf *RESTRICT, arrayf &RESTRICT, arrayi &RESTRICT,
          int num, const aabb &RESTRICT);

