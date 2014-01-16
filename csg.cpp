/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#include "csg.hpp"
#include "sys.hpp"
#include "math.hpp"

namespace q {
namespace csg {

/*--------------------------------------------------------------------------
 - expose all basic CSG nodes
 -------------------------------------------------------------------------*/
enum {
  UNION, DIFFERENCE, INTERSECTION,
  SPHERE, BOX, PLANE, CYLINDER,
  TRANSLATION
};
struct node {
  INLINE node(u32 type) : type(type) {}
  u32 type;
};
#define BINARY(NAME,TYPE) \
struct NAME : node { \
  INLINE NAME(node *left = NULL, node *right = NULL) : \
    node(TYPE), left(left), right(right) {}\
  node *left, *right; \
};
BINARY(U,UNION)
BINARY(D,DIFFERENCE)
BINARY(I,INTERSECTION)
#undef BINARY

struct box : node {
  INLINE box(const vec3f &extent) : node(BOX), extent(extent) {}
  vec3f extent;
};
struct plane : node {
  INLINE plane(const vec4f &p) : node(PLANE), p(p) {}
  vec4f p;
};
struct sphere : node {
  INLINE sphere(float r) : node(SPHERE), r(r) {}
  float r;
};
struct cylinder : node {
  INLINE cylinder(const vec2f &cxz, float r) : node(CYLINDER), cxz(cxz), r(r) {}
  vec2f cxz;
  float r;
};
struct translation : node {
  INLINE translation(const vec3f &p, node *n = NULL) :
    node(TRANSLATION), p(p), n(n) {}
  vec3f p;
  node *n;
};

node *capped_cylinder(const vec2f &cxz, const vec3f &ryminymax) {
  const auto cyl = NEW(cylinder, cxz, ryminymax.x);
  const auto plane0 = NEW(plane, vec4f(0.f,1.f,0.f,-ryminymax.y));
  const auto plane1 = NEW(plane, vec4f(0.f,-1.f,0.f,ryminymax.z));
  return NEW(D, NEW(D, cyl, plane0), plane1);
}

node *makescene() {
  const auto s = NEW(sphere, 4.2f);
  const auto d0 = NEW(translation, vec3f(7.f, 5.f, 7.f), s);
  const auto d1 = NEW(box, vec3f(4.f));
  node *c = NEW(U, d0, d1);
  loopi(16) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2*float(i)+2.f);
    c = NEW(U, c, capped_cylinder(center, ryminymax));
  }
  const auto b = NEW(box, vec3f(2.f, 5.f, 18.f));
  return NEW(D, c, NEW(translation, vec3f(2.f,5.f,18.f), b));
}

float dist(const vec3f &pos, node *n) {
  switch (n->type) {
    case UNION: {
      const auto u = static_cast<U*>(n);
      return min(dist(pos, u->left), dist(pos, u->right));
    }
    case INTERSECTION: {
      const auto i = static_cast<I*>(n);
      return max(dist(pos, i->left), dist(pos, i->right));
    }
    case DIFFERENCE: {
      const auto d = static_cast<D*>(n);
      return min(dist(pos, d->left), -dist(pos, d->right));
    }
    case TRANSLATION: {
      const auto t = static_cast<translation*>(n);
      return dist(pos-t->p, t->n);
    }
    case PLANE: {
      const auto p = static_cast<plane*>(n);
      return dot(pos, p->p.xyz()) + p->p.w;
    }
    case CYLINDER: {
      const auto c = static_cast<cylinder*>(n);
      return length(pos.xz()-c->cxz) - c->r;
    }
    case SPHERE: {
      const auto s = static_cast<sphere*>(n);
      return length(pos) - s->r;
    }
    case BOX: {
      const vec3f d = abs(pos)-static_cast<box*>(n)->extent;;
      return min(max(d.x,max(d.y,d.z)),0.0f) + length(max(d,vec3f(zero)));
    }
  }
  assert("unreachable" && false);
  return FLT_MAX;
}
} /* namespace csg */
} /* namespace q */

