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

// shadow ray packet generation
void shadowpacket(const raypacket &RESTRICT primary,
                  const packethit &RESTRICT hit,
                  const vec3f &RESTRICT lpos,
                  raypacket &RESTRICT shadow,
                  packetshadow &RESTRICT occluded);

// clear and initialize packethit
void clearpackethit(packethit &hit);

// frame buffer write (rgb normal)
void writenormal(const packethit &RESTRICT hit,
                 const vec2i &RESTRICT tileorg,
                 const vec2i &RESTRICT screensize,
                 int *RESTRICT pixels);

// frame buffer write (just simple dot product)
void writendotl(const raypacket &RESTRICT shadow,
                const packethit &RESTRICT hit,
                const packetshadow &RESTRICT occluded,
                const vec2i &RESTRICT tileorg,
                const vec2i &RESTRICT screensize,
                int *RESTRICT pixels);

// normalize geometric normal in the packet hit structure
void normalizehitnormal(packethit &hit, int raynum);

