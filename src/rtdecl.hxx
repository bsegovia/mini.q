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
void shadowpacket(const array3f &RESTRICT pos,
                  const arrayi &RESTRICT mask,
                  const vec3f &RESTRICT lpos,
                  raypacket &RESTRICT shadow,
                  packetshadow &RESTRICT occluded,
                  int raynum);

// compute normals and position of primary hit points
u32 primarypoint(const raypacket &RESTRICT p,
                 const packethit &RESTRICT hit,
                 array3f &RESTRICT pos,
                 array3f &RESTRICT nor,
                 arrayi &RESTRICT mask);

// clear and initialize packethit
void clearpackethit(packethit &hit);

// frame buffer write (rgb normal)
void writenormal(const packethit &RESTRICT hit,
                 const vec2i &RESTRICT tileorg,
                 const vec2i &RESTRICT screensize,
                 int *RESTRICT pixels);

// frame buffer write (just simple dot product)
void writendotl(const raypacket &RESTRICT shadow,
                const array3f &RESTRICT nor,
                const packetshadow &RESTRICT occluded,
                const vec2i &RESTRICT tileorg,
                const vec2i &RESTRICT screensize,
                int *RESTRICT pixels);

// zero clear the given tile
void clear(const vec2i &RESTRICT tileorg,
           const vec2i &RESTRICT screensize,
           int *RESTRICT pixels);

