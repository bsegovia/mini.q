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
  virtual ~node() {}
  u32 type;
};
#define BINARY(NAME,TYPE) \
struct NAME : node { \
  INLINE NAME(node *left = NULL, node *right = NULL) : \
    node(TYPE), left(left), right(right) {}\
  virtual ~NAME() {SAFE_DEL(left); SAFE_DEL(right);}\
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
  virtual ~translation() { SAFE_DEL(n); }
  vec3f p;
  node *n;
};

#if 1
node *capped_cylinder(const vec2f &cxz, const vec3f &ryminymax) {
  const auto cyl = NEW(cylinder, cxz, ryminymax.x);
  const auto plane0 = NEW(plane, vec4f(0.f,1.f,0.f,-ryminymax.y));
  const auto plane1 = NEW(plane, vec4f(0.f,-1.f,0.f,ryminymax.z));
  return NEW(D, NEW(D, cyl, plane0), plane1);
}
#else
node *capped_cylinder(const vec2f &cxz, const vec3f &ryminymax) {
  const auto cyl = NEW(cylinder, cxz, ryminymax.x);
  const auto box = NEW(box, vec4f(0.f,1.f,0.f,-ryminymax.y));
  return NEW(D, NEW(D, cyl, plane0), plane1);
}
#endif

static node *makescene0() {
#if 1
  const auto t = vec3f(7.f, 5.f, 7.f);
  const auto s = NEW(sphere, 4.2f);
  const auto b0 = NEW(box, vec3f(4.f));
  const auto d0 = NEW(translation, t, s);
  const auto d1 = NEW(translation, t, b0);
  // node *c = NULL; //NEW(D, d1, d0);
  node *c = NULL; NEW(D, d1, d0);
  loopi(16) {
  // for (int i = 11; i < 16; ++i) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2*float(i)+2.f);
    if (c == NULL)
        c = capped_cylinder(center, ryminymax);
      else
        c = NEW(U, c, capped_cylinder(center, ryminymax));
  }
  const auto b = NEW(box, vec3f(3.5f, 4.f, 3.5f));
  return NEW(D, c, NEW(translation, vec3f(2.f,5.f,18.f), b));
#else
  node *c = NULL;
  loopi(3) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2.f*float(i)+2.f);
    if (c == NULL)
      c = capped_cylinder(center, ryminymax);
    else
      c = NEW(U, c, capped_cylinder(center, ryminymax));
  }
  const auto b = NEW(box, vec3f(3.5f, 4.f, 3.5f));
  return NEW(D, c, NEW(translation, vec3f(2.f,5.f,18.f), b));

#endif
}

node *makescene() {
  node *s0 = makescene0();
#if 0
  node *s1 = NEW(translation, vec3f(8.f,0.f,0.f), makescene0());
  node *s2 = NEW(translation, vec3f(16.f,0.f,0.f), makescene0());
  node *s3 = NEW(translation, vec3f(24.f,0.f,0.f), makescene0());
  return NEW(U, NEW(U, s2, s3), NEW(U, s0, s1));
#else
  return s0;
#endif
}

void destroyscene(node *n) { SAFE_DEL(n); }

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
      return max(dist(pos, d->left), -dist(pos, d->right));
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
      return 0.2f*sin(4.f*pos.y+c->cxz.x) + length(pos.xz()-c->cxz) - c->r;
    }
    case SPHERE: {
      const auto s = static_cast<sphere*>(n);
      return length(pos) - s->r;
    }
    case BOX: {
      const vec3f d = abs(pos)-static_cast<box*>(n)->extent;
      return min(max(d.x,max(d.y,d.z)),0.0f) + length(max(d,vec3f(zero)));
    }
  }
  assert("unreachable" && false);
  return FLT_MAX;
}
} /* namespace csg */
} /* namespace q */

