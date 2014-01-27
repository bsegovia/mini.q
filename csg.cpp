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
  SPHERE, BOX, PLANE, CYLINDERXZ, CYLINDERYZ, CYLINDERXY,
  TRANSLATION
};
struct node {
  INLINE node(u32 type, const aabb &box = aabb::empty()) :
    box(box), type(type) {}
  virtual ~node() {}
  aabb box;
  u32 type;
};
#define BINARY(NAME,TYPE,BOX) \
struct NAME : node { \
  INLINE NAME(node &left, node &right) :\
    node(TYPE, BOX), left(&left), right(&right) {}\
  virtual ~NAME() {SAFE_DEL(left); SAFE_DEL(right);}\
  node *left, *right; \
};
BINARY(U,UNION, sum(left.box, right.box))
BINARY(D,DIFFERENCE, left.box)
BINARY(I,INTERSECTION, intersection(left.box, right.box))
#undef BINARY

struct box : node {
  INLINE box(const vec3f &extent) :
    node(BOX, aabb(-extent,+extent)), extent(extent) {}
  vec3f extent;
};
struct plane : node {
  INLINE plane(const vec4f &p) : node(PLANE, aabb::all()), p(p) {}
  vec4f p;
};
struct sphere : node {
  INLINE sphere(float r) : node(SPHERE, aabb(-r, r)), r(r) {}
  float r;
};
struct cylinderxz : node {
  INLINE cylinderxz(const vec2f &cxz, float r) :
    node(CYLINDERXZ, aabb(vec3f(-r+cxz.x,-FLT_MAX,-r+cxz.y),
                          vec3f(+r+cxz.x,+FLT_MAX,+r+cxz.y))), cxz(cxz), r(r) {}
  vec2f cxz;
  float r;
};
struct cylinderxy : node {
  INLINE cylinderxy(const vec2f &cxy, float r) :
    node(CYLINDERXY, aabb(vec3f(-r+cxy.x,-r+cxy.y,-FLT_MAX),
                          vec3f(+r+cxy.x,+r+cxy.y,+FLT_MAX))), cxy(cxy), r(r) {}
  vec2f cxy;
  float r;
};
struct cylinderyz : node {
  INLINE cylinderyz(const vec2f &cyz, float r) :
    node(CYLINDERYZ, aabb(vec3f(-FLT_MAX,-r+cyz.x,-r+cyz.y),
                          vec3f(+FLT_MAX,+r+cyz.x,+r+cyz.y))), cyz(cyz), r(r) {}
  vec2f cyz;
  float r;
};
struct translation : node {
  INLINE translation(const vec3f &p, node &n) :
    node(TRANSLATION, aabb(p+n.box.pmin, p+n.box.pmax)), p(p), n(&n) {}
  virtual ~translation() { SAFE_DEL(n); }
  vec3f p;
  node *n;
};

#if 1
node *capped_cylinder(const vec2f &cxz, const vec3f &ryminymax) {
  const auto r = ryminymax.x;
  const auto ymin = ryminymax.y;
  const auto ymax = ryminymax.z;
  const auto cyl = NEW(cylinderxz, cxz, r);
  const auto plane0 = NEW(plane, vec4f(0.f,1.f,0.f,-ymin));
  const auto plane1 = NEW(plane, vec4f(0.f,-1.f,0.f,ymax));
  const auto ccyl = NEW(D, *NEW(D, *cyl, *plane0), *plane1);
  ccyl->box.pmin = vec3f(cxz.x-r,ymin,cxz.y-r);
  ccyl->box.pmax = vec3f(cxz.x+r,ymax,cxz.y+r);
  return ccyl;
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
  const auto d0 = NEW(translation, t, *s);
  const auto d1 = NEW(translation, t, *b0);
  node *c = NEW(D, *d1, *d0);
  loopi(16) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2*float(i)+2.f);
    if (c == NULL)
        c = capped_cylinder(center, ryminymax);
      else
        c = NEW(U, *c, *capped_cylinder(center, ryminymax));
  }
  const auto b = NEW(box, vec3f(3.5f, 4.f, 3.5f));
  const auto scene0 = NEW(D, *c, *NEW(translation, vec3f(2.f,5.f,18.f), *b));

  // build an arcade here
  node *big = NEW(box, vec3f(3.f, 4.0f, 20.f));
  node *cut = NEW(translation, vec3f(0.f,-2.f,0.f), *NEW(box, vec3f(2.f, 2.f, 20.f)));
  big = NEW(D, *big, *cut);
  big = NEW(translation, vec3f(16.f,4.f,10.f), *big);
  node *cxy = NEW(cylinderxy, vec2f(zero), 2.f);
  cxy = NEW(translation, vec3f(16.f, 4.f, 10.f), *cxy);
  node *arcade = NEW(D, *big, *cxy);
  loopi(7) {
    const auto pos = vec3f(16.f,3.5f,7.f+3.f*float(i));
    const auto hole = NEW(box, vec3f(3.f,1.f,1.f));
    arcade = NEW(D, *arcade, *NEW(translation, pos, *hole));
  }

  // just make a union of them
  return NEW(U, *scene0, *arcade);
}

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
}
#endif

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

void distr(const node &n, const vec3f *pos, float *dist, int num, const aabb &box) {
  const auto isec = intersection(box, n.box);
  if (any(gt(isec.pmin, isec.pmax))) return;
  switch (n.type) {
    case UNION: {
      const auto &u = static_cast<const U&>(n);
      float tempdist[64];
      distr(*u.left, pos, dist, num, box);
      loopi(num) tempdist[i] = FLT_MAX;
      distr(*u.right, pos, tempdist, num, box);
      loopi(num) dist[i] = min(dist[i], tempdist[i]);
      return;
    }
    case INTERSECTION: {
      const auto &i = static_cast<const I&>(n);
      float tempdist[64];
      distr(*i.left, pos, dist, num, box);
      loopi(num) tempdist[i] = FLT_MAX;
      distr(*i.right, pos, tempdist, num, box);
      loopi(num) dist[i] = max(dist[i], tempdist[i]);
      return;
    }
    case DIFFERENCE: {
      const auto &d = static_cast<const D&>(n);
      float tempdist[64];
      distr(*d.left, pos, dist, num, box);
      loopi(num) tempdist[i] = FLT_MAX;
      distr(*d.right, pos, tempdist, num, box);
      loopi(num) dist[i] = max(dist[i], -tempdist[i]);
      return;
    }
    case TRANSLATION: {
      const auto &t = static_cast<const translation&>(n);
      vec3f tpos[64];
      loopi(num) tpos[i] = pos[i] - t.p;
      distr(*t.n, tpos, dist, num, aabb(box.pmin-t.p, box.pmax-t.p));
      return;
    }
    case PLANE: {
      const auto &p = static_cast<const plane&>(n);
      loopi(num) dist[i] = dot(pos[i], p.p.xyz()) + p.p.w;
      return;
    }
    case CYLINDERXZ: {
      const auto &c = static_cast<const cylinderxz&>(n);
      loopi(num) dist[i] = length(pos[i].xz()-c.cxz) - c.r;
      return;
    }
    case CYLINDERXY: {
      const auto &c = static_cast<const cylinderxy&>(n);
      loopi(num) dist[i] = length(pos[i].xy()-c.cxy) - c.r;
      return;
    }
    case CYLINDERYZ: {
      const auto &c = static_cast<const cylinderyz&>(n);
      loopi(num) dist[i] = length(pos[i].yz()-c.cyz) - c.r;
      return;
    }
    case SPHERE: {
      const auto &s = static_cast<const sphere&>(n);
      loopi(num) dist[i] = length(pos[i]) - s.r;
      return;
    }
    case BOX: {
      const auto extent = static_cast<const struct box&>(n).extent;
      loopi(num) {
        const auto pd = abs(pos[i])-extent;
        dist[i] = min(max(pd.x,max(pd.y,pd.z)),0.0f) + length(max(pd,vec3f(zero)));
      }
      return;
    }
  }
  assert("unreachable" && false);
}

void dist(const node &n, const vec3f *pos, float *d, int num, const aabb &box) {
  assert(num <= 64);
  loopi(num) d[i] = FLT_MAX;
  distr(n, pos, d, num, box);
}

float dist(const node &n, const vec3f &pos, const aabb &box) {
  const auto isec = intersection(box, n.box);
  if (any(gt(isec.pmin, isec.pmax))) return FLT_MAX;
  switch (n.type) {
    case UNION: {
      const auto &u = static_cast<const U&>(n);
      return min(dist(*u.left, pos, box), dist(*u.right, pos, box));
    }
    case INTERSECTION: {
      const auto &i = static_cast<const I&>(n);
      return max(dist(*i.left, pos, box), dist(*i.right, pos, box));
    }
    case DIFFERENCE: {
      const auto &d = static_cast<const D&>(n);
      return max(dist(*d.left, pos, box), -dist(*d.right, pos, box));
    }
    case TRANSLATION: {
      const auto &t = static_cast<const translation&>(n);
      return dist(*t.n, pos-t.p, aabb(box.pmin-t.p, box.pmax-t.p));
    }
    case PLANE: {
      const auto &p = static_cast<const plane&>(n);
      return dot(pos, p.p.xyz()) + p.p.w;
    }
    case CYLINDERXZ: {
      const auto &c = static_cast<const cylinderxz&>(n);
      return length(pos.xz()-c.cxz) - c.r;
    }
    case CYLINDERXY: {
      const auto &c = static_cast<const cylinderxy&>(n);
      return length(pos.xy()-c.cxy) - c.r;
    }
    case CYLINDERYZ: {
      const auto &c = static_cast<const cylinderyz&>(n);
      return length(pos.yz()-c.cyz) - c.r;
    }
    case SPHERE: {
      const auto &s = static_cast<const sphere&>(n);
      return length(pos) - s.r;
    }
    case BOX: {
      const auto &d = abs(pos)-static_cast<const struct box&>(n).extent;
      return min(max(d.x,max(d.y,d.z)),0.0f) + length(max(d,vec3f(zero)));
    }
  }
  assert("unreachable" && false);
  return FLT_MAX;
}
} /* namespace csg */
} /* namespace q */

