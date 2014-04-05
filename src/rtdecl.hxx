/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - rtdecl.hxx -> template to declare various ray tracing routines
 -------------------------------------------------------------------------*/
// ray tracing routines (visiblity and shadow rays)
void closest(const struct intersector&, const struct raypacket&, struct packethit&);
void occluded(const struct intersector&, const struct raypacket&, struct packetshadow&);

// ray packet generation
void visibilitypacket(const struct camera &RESTRICT cam,
                      struct raypacket &RESTRICT p,
                      const vec2i &RESTRICT tileorg,
                      const vec2i &RESTRICT screensize);
void visibilitypackethit(packethit &hit);

// frame buffer write
void fbwritenormal(const packethit &RESTRICT hit,
                   const vec2i &RESTRICT tileorg,
                   const vec2i &RESTRICT screensize,
                   int *RESTRICT pixels);

