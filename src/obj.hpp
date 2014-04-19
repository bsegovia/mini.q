/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - obj.cpp -> exposes routines to load Maya .obj files
 -------------------------------------------------------------------------*/
#pragma once
#include "base/sys.hpp"
#include "base/math.hpp"
#include "base/utility.hpp"

namespace q {

// generic 3d object file directly taken from wavefront .obj (but vertices are
// processed and merged here)
struct obj : noncopyable {
  struct triangle {
    INLINE triangle(void) {}
    INLINE triangle(vec3i v, int m) : v(v), m(m) {}
    vec3i v;
    int m;
  };
  struct vertex {
    INLINE vertex(void) {}
    INLINE vertex(vec3f p, vec3f n, vec2f t) : p(p), n(n), t(t) {}
    vec3f p, n;
    vec2f t;
  };
  struct matgroup {
    INLINE matgroup(void) {}
    INLINE matgroup(int first, int last, int m) : first(first), last(last), m(m) {}
    int first, last, m;
  };
  struct material {
    char *name;
    char *mapka;
    char *mapkd;
    char *mapd;
    char *mapbump;
    double amb[3];
    double diff[3];
    double spec[3];
    double km;
    double reflect;
    double refract;
    double trans;
    double shiny;
    double glossy;
    double refract_index;
  };

  obj(void);
  ~obj(void);
  INLINE bool valid(void) const { return trinum > 0; }
  bool load(const char *path);
  triangle *tri;
  vertex *vert;
  matgroup *grp;
  material *mat;
  u32 trinum;
  u32 vertnum;
  u32 grpnum;
  u32 matnum;
};
} // namespace q

