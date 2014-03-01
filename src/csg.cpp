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
 - all materials
 -------------------------------------------------------------------------*/
const material airmat(MAT_AIR);
const material snoisemat(MAT_SNOISE);
const material gridmat(MAT_GRID);

/*--------------------------------------------------------------------------
 - expose all basic CSG nodes
 -------------------------------------------------------------------------*/
enum CSGOP {
  C_UNION, C_DIFFERENCE, C_INTERSECTION,
  C_SPHERE, C_BOX, C_PLANE, C_CYLINDERXZ, C_CYLINDERYZ, C_CYLINDERXY,
  C_TRANSLATION, C_ROTATION,
  C_INVALID = 0xffffffff
};
struct node {
  INLINE node(CSGOP type, const aabb &box = aabb::empty()) : box(box), type(type) {}
  virtual ~node() {}
  aabb box;
  CSGOP type;
};
#define BINARY(NAME,TYPE,C_BOX) \
struct NAME : node { \
  INLINE NAME(node *nleft, node *nright) :\
    node(TYPE, C_BOX), left(nleft), right(nright) {}\
  virtual ~NAME() {SAFE_DEL(left); SAFE_DEL(right);}\
  node *left, *right; \
};
BINARY(U,C_UNION, sum(nleft->box, nright->box))
BINARY(D,C_DIFFERENCE, nleft->box)
BINARY(I,C_INTERSECTION, intersection(nleft->box, nright->box))
#undef BINARY

struct box : node {
  INLINE box(const vec3f &extent) :
    node(C_BOX, aabb(-extent,+extent)), extent(extent) {}
  vec3f extent;
};
struct plane : node {
  INLINE plane(const vec4f &p) : node(C_PLANE, aabb::all()), p(p) {}
  vec4f p;
};
struct sphere : node {
  INLINE sphere(float r) : node(C_SPHERE, aabb(-r, r)), r(r) {}
  float r;
};
struct cylinderxz : node {
  INLINE cylinderxz(const vec2f &cxz, float r) :
    node(C_CYLINDERXZ, aabb(vec3f(-r+cxz.x,-FLT_MAX,-r+cxz.y),
                            vec3f(+r+cxz.x,+FLT_MAX,+r+cxz.y))), cxz(cxz), r(r)
  {}
  vec2f cxz;
  float r;
};
struct cylinderxy : node {
  INLINE cylinderxy(const vec2f &cxy, float r) :
    node(C_CYLINDERXY, aabb(vec3f(-r+cxy.x,-r+cxy.y,-FLT_MAX),
                            vec3f(+r+cxy.x,+r+cxy.y,+FLT_MAX))), cxy(cxy), r(r)
  {}
  vec2f cxy;
  float r;
};
struct cylinderyz : node {
  INLINE cylinderyz(const vec2f &cyz, float r) :
    node(C_CYLINDERYZ, aabb(vec3f(-FLT_MAX,-r+cyz.x,-r+cyz.y),
                            vec3f(+FLT_MAX,+r+cyz.x,+r+cyz.y))), cyz(cyz), r(r)
  {}
  vec2f cyz;
  float r;
};
struct translation : node {
  INLINE translation(const vec3f &p, node *n) :
    node(C_TRANSLATION, aabb(p+n->box.pmin, p+n->box.pmax)), p(p), n(n) {}
  virtual ~translation() { SAFE_DEL(n); }
  vec3f p;
  node *n;
};
struct rotation : node {
  INLINE rotation(const quat3f &q, node *n) :
    node(C_ROTATION, aabb::all()), q(q), n(n) {}
  virtual ~rotation() { SAFE_DEL(n); }
  quat3f q;
  node *n;
};

node *capped_cylinder(const vec2f &cxz, const vec3f &ryminymax) {
  const auto r = ryminymax.x;
  const auto ymin = ryminymax.y;
  const auto ymax = ryminymax.z;
  const auto cyl = NEW(cylinderxz, cxz, r);
  const auto plane0 = NEW(plane, vec4f(0.f,1.f,0.f,-ymin));
  const auto plane1 = NEW(plane, vec4f(0.f,-1.f,0.f,ymax));
  const auto ccyl = NEW(D, NEW(D, cyl, plane0), plane1);
  ccyl->box.pmin = vec3f(cxz.x-r,ymin,cxz.y-r);
  ccyl->box.pmax = vec3f(cxz.x+r,ymax,cxz.y+r);
  return ccyl;
}
#if 1
static node *makescene0() {
  const auto t = vec3f(7.f, 5.f, 7.f);
  const auto s = NEW(sphere, 4.2f);
  //const auto q = quat3f(deg2rad(20.0f),deg2rad(25.f),0.f);
  //const auto b0 = NEW(rotation, q, NEW(box, vec3f(4.f)));
  const auto b0 = NEW(box, vec3f(4.f));
  const auto d0 = NEW(translation, t, s);
  const auto d1 = NEW(translation, t, b0);
  node *c = NEW(D, d1, d0);
  loopi(16) {
    const auto center = vec2f(2.f,2.f+2.f*float(i));
    const auto ryminymax = vec3f(1.f,1.f,2*float(i)+2.f);
    if (c == NULL)
        c = capped_cylinder(center, ryminymax);
      else
        c = NEW(U, c, capped_cylinder(center, ryminymax));
  }
  const auto b = NEW(box, vec3f(3.5f, 4.f, 3.5f));
  const auto scene0 = NEW(D, c, NEW(translation, vec3f(2.f,5.f,18.f), b));

  // build an arcade here
  node *big = NEW(box, vec3f(3.f, 4.0f, 20.f));
  node *cut = NEW(translation, vec3f(0.f,-2.f,0.f), NEW(box, vec3f(2.f, 2.f, 20.f)));
  big = NEW(D, big, cut);
  node *cxy = NEW(cylinderxy, vec2f(zero), 2.f);
 // cxy = NEW(translation, vec3f(16.f, 4.f, 10.f), *cxy);
  node *arcade = NEW(D, big, cxy);
  //arcade = NEW(rotation, quat3f(20.f,0.f,0.f), arcade);
  arcade = NEW(translation, vec3f(16.f,4.f,10.f), arcade);
  loopi(7) {
    const auto pos = vec3f(16.f,3.5f,7.f+3.f*float(i));
    const auto hole = NEW(box, vec3f(3.f,1.f,1.f));
    arcade = NEW(D, arcade, NEW(translation, pos, hole));
  }

  // add a ground box
  const auto groundbox = NEW(box, vec3f(50.f, 4.f, 50.f));
  const auto ground = NEW(translation, vec3f(0.f,-3.f,0.f), groundbox);

  // just make a union of them
  return NEW(U, ground, NEW(U, scene0, arcade));
}
#else
static node *makescene0() {
  const auto b0 = NEW(box, vec3f(40.f, 4.f, 40.f));
  const auto b1 = NEW(box, vec3f(2.f, 8.f, 2.f));
  const auto t = NEW(translation, vec3f(20.f,0.f,20.f), b1);
  const auto d = NEW(translation, vec3f(0.f,-3.f,0.f), NEW(U, b0, t));
  return d;
}
#endif

node *makescene() {
  node *s0 = makescene0();
  int k = 0, j = 0;
  loopi(16) // loopj(2) loopk(2)
  s0 = NEW(U, s0, NEW(translation, vec3f(float(i)*30.f,float(k)*40.f,float(j)*20.f), makescene0()));
  return s0;
}

void destroyscene(node *n) { SAFE_DEL(n); }

void distr(const node *n, const vec3f * RESTRICT pos, float * RESTRICT dist, int num, const aabb &box) {

  switch (n->type) {
    case C_UNION: {
      const auto u = static_cast<const U*>(n);
      const auto isecleft = intersection(u->left->box, box);
      const auto isecright = intersection(u->right->box, box);
      const auto goleft = all(le(isecleft.pmin, isecleft.pmax));
      const auto goright = all(le(isecright.pmin, isecright.pmax));
      if (goleft && goright) {
        float tempdist[64];
        distr(u->left, pos, dist, num, box);
        loopi(num) tempdist[i] = FLT_MAX;
        distr(u->right, pos, tempdist, num, box);
        loopi(num) dist[i] = min(dist[i], tempdist[i]);
      } else if (goleft)
        distr(u->left, pos, dist, num, box);
      else if (goright)
        distr(u->right, pos, dist, num, box);
      break;
    }
    case C_INTERSECTION: {
      const auto i = static_cast<const I*>(n);
      const auto isecleft = intersection(i->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      const auto isecright = intersection(i->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      float tempdist[64];
      distr(i->left, pos, dist, num, box);
      loopi(num) tempdist[i] = FLT_MAX;
      distr(i->right, pos, tempdist, num, box);
      loopi(num) dist[i] = max(dist[i], tempdist[i]);
      break;
    }
    case C_DIFFERENCE: {
      const auto d = static_cast<const D*>(n);
      const auto isecleft = intersection(d->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      distr(d->left, pos, dist, num, box);

      const auto isecright = intersection(d->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      float tempdist[64];
      loopi(num) tempdist[i] = FLT_MAX;
      distr(d->right, pos, tempdist, num, box);
      loopi(num) dist[i] = max(dist[i], -tempdist[i]);
      break;
    }
    case C_TRANSLATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto t = static_cast<const translation*>(n);
      vec3f tpos[64];
      loopi(num) tpos[i] = pos[i] - t->p;
      distr(t->n, tpos, dist, num, aabb(box.pmin-t->p, box.pmax-t->p));
      break;
    }
    case C_ROTATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto r = static_cast<const rotation*>(n);
      vec3f tpos[64];
      loopi(num) tpos[i] = xfmpoint(conj(r->q), pos[i]);
      distr(r->n, tpos, dist, num, aabb::all());
      break;
    }
    case C_PLANE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto p = static_cast<const plane*>(n);
      loopi(num) dist[i] = dot(pos[i], p->p.xyz()) + p->p.w;
      break;
    }

#define CYL(NAME, COORD)\
  case C_CYLINDER##NAME: {\
    const auto isec = intersection(n->box, box);\
    if (any(gt(isec.pmin, isec.pmax))) break;\
    const auto c = static_cast<const cylinder##COORD*>(n);\
    loopi(num) dist[i] = length(pos[i].COORD()-c->c##COORD) - c->r;\
    break;\
  }
  CYL(XY,xy); CYL(XZ,xz); CYL(YZ,yz);
#undef CYL

    case C_SPHERE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto s = static_cast<const sphere*>(n);
      loopi(num) dist[i] = length(pos[i]) - s->r;
      break;
    }
    case C_BOX: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto extent = static_cast<const struct box*>(n)->extent;
      loopi(num) {
        const auto pd = abs(pos[i])-extent;
        dist[i] = min(max(pd.x,max(pd.y,pd.z)),0.0f) + length(max(pd,vec3f(zero)));
      }
      break;
    }
    case C_INVALID: assert("unreachable" && false);
  }
}

void dist(const node *n, const vec3f *pos, float *d, int num, const aabb &box) {
  assert(num <= 64);
  loopi(num) d[i] = FLT_MAX;
  distr(n, pos, d, num, box);
}

float dist(const node *n, const vec3f &pos, const aabb &box) {
  const auto isec = intersection(box, n->box);
  if (any(gt(isec.pmin, isec.pmax))) return FLT_MAX;
  switch (n->type) {
    case C_UNION: {
      const auto u = static_cast<const U*>(n);
      const auto left = dist(u->left, pos, box);
      const auto right = dist(u->right, pos, box);
      return min(left,right);
    }
    case C_INTERSECTION: {
      const auto i = static_cast<const I*>(n);
      const auto left = dist(i->left, pos, box);
      const auto right = dist(i->right, pos, box);
      return max(left,right);
    }
    case C_DIFFERENCE: {
      const auto d = static_cast<const D*>(n);
      const auto left = dist(d->left, pos, box);
      const auto right = dist(d->right, pos, box);
      return max(left,-right);
    }
    case C_TRANSLATION: {
      const auto t = static_cast<const translation*>(n);
      return dist(t->n, pos-t->p, aabb(box.pmin-t->p, box.pmax-t->p));
    }
    case C_ROTATION: {
      const auto r = static_cast<const rotation*>(n);
      return dist(r->n, xfmpoint(conj(r->q), pos), aabb::all());
    }
    case C_PLANE: {
      const auto p = static_cast<const plane*>(n);
      return dot(pos, p->p.xyz()) + p->p.w;
    }
    case C_CYLINDERXZ: {
      const auto c = static_cast<const cylinderxz*>(n);
      return length(pos.xz()-c->cxz) - c->r;
    }
    case C_CYLINDERXY: {
      const auto c = static_cast<const cylinderxy*>(n);
      return length(pos.xy()-c->cxy) - c->r;
    }
    case C_CYLINDERYZ: {
      const auto c = static_cast<const cylinderyz*>(n);
      return length(pos.yz()-c->cyz) - c->r;
    }
    case C_SPHERE: {
      const auto s = static_cast<const sphere*>(n);
      return length(pos) - s->r;
    }
    case C_BOX: {
      const auto &d = abs(pos)-static_cast<const struct box*>(n)->extent;
      const auto dist = min(max(d.x,max(d.y,d.z)),0.0f)+length(max(d,vec3f(zero)));
      return dist;
    }
    default: assert("unreachable" && false); return FLT_MAX;
  }
}
} /* namespace csg */
} /* namespace q */

