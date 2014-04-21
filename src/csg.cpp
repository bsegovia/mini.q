/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csg.cpp -> implements csg routines
 -------------------------------------------------------------------------*/
#include "csg.hpp"
#include "base/math.hpp"
#include "base/script.hpp"
#include "base/sys.hpp"

namespace q {
namespace csg {

/*--------------------------------------------------------------------------
 - implements all basic CSG nodes
 -------------------------------------------------------------------------*/
struct emptynode : node {
  INLINE emptynode() : node(C_EMPTY) {}
};
// we need to be careful with null nods
static INLINE aabb fixedaabb(const ref<node> &n) {
  return n.ptr?n->box:aabb::empty();
}
static INLINE ref<node> fixednode(const ref<node> &n) {
  return NULL!=n.ptr?n:NEWE(emptynode);
}

#define BINARY(NAME,TYPE,C_BOX) \
struct NAME : node { \
  INLINE NAME(const ref<node> &nleft, const ref<node> &nright) :\
    node(TYPE, C_BOX), left(fixednode(nleft)), right(fixednode(nright)) {}\
  ref<node> left, right; \
};
BINARY(U, C_UNION, sum(fixedaabb(nleft), fixedaabb(nright)))
BINARY(D, C_DIFFERENCE, fixedaabb(nleft))
BINARY(I, C_INTERSECTION, intersection(fixedaabb(nleft), fixedaabb(nright)))
BINARY(R, C_REPLACE, fixedaabb(nleft));
#undef BINARY

struct materialnode : node {
  INLINE materialnode(CSGOP type, const aabb &box = aabb::empty(), u32 matindex = MAT_SIMPLE_INDEX) :
    node(type, box), matindex(matindex) {}
  u32 matindex;
};

struct box : materialnode {
  INLINE box(const vec3f &extent, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_BOX, aabb(-extent,+extent), matindex), extent(extent) {}
  INLINE box(float x, float y, float z, u32 matindex = MAT_SIMPLE_INDEX) :
    box(vec3f(x,y,z), matindex) {}
  vec3f extent;
};
struct plane : materialnode {
  INLINE plane(const vec4f &p, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_PLANE, aabb::all(), matindex), p(p) {}
  INLINE plane(float a, float b, float c, float d, u32 matindex = MAT_SIMPLE_INDEX) :
    plane(vec4f(a,b,c,d), matindex) {}
  vec4f p;
};
struct sphere : materialnode {
  INLINE sphere(float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_SPHERE, aabb(-r, r), matindex), r(r) {}
  float r;
};

INLINE aabb cyzbox(const vec2f &cyz, float r) {
  return aabb(vec3f(-FLT_MAX,-r+cyz.x,-r+cyz.y),
              vec3f(+FLT_MAX,+r+cyz.x,+r+cyz.y));;
}
INLINE aabb cxzbox(const vec2f &cxz, float r) {
  return aabb(vec3f(-r+cxz.x,-FLT_MAX,-r+cxz.y),
              vec3f(+r+cxz.x,+FLT_MAX,+r+cxz.y));
}
INLINE aabb cxybox(const vec2f &cxy, float r) {
  return aabb(vec3f(-r+cxy.x,-r+cxy.y,-FLT_MAX),
              vec3f(+r+cxy.x,+r+cxy.y,+FLT_MAX));
}

struct cylinderxz : materialnode {
  INLINE cylinderxz(const vec2f &cxz, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERXZ, cxzbox(cxz,r), matindex), cxz(cxz), r(r) {}
  INLINE cylinderxz(float x, float z, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderxz(vec2f(x,z), r, matindex) {}
  vec2f cxz;
  float r;
};
struct cylinderxy : materialnode {
  INLINE cylinderxy(const vec2f &cxy, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERXY, cxybox(cxy,r), matindex), cxy(cxy), r(r) {}
  INLINE cylinderxy(float x, float y, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderxy(vec2f(x,y), r, matindex) {}
  vec2f cxy;
  float r;
};
struct cylinderyz : materialnode {
  INLINE cylinderyz(const vec2f &cyz, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    materialnode(C_CYLINDERYZ, cyzbox(cyz,r), matindex), cyz(cyz), r(r) {}
  INLINE cylinderyz(float y, float z, float r, u32 matindex = MAT_SIMPLE_INDEX) :
    cylinderyz(vec2f(y,z), r, matindex) {}
  vec2f cyz;
  float r;
};
struct translation : node {
  INLINE translation(const vec3f &p, const ref<node> &n) :
    node(C_TRANSLATION, aabb(p+fixedaabb(n).pmin, p+fixedaabb(n).pmax)), p(p),
    n(fixednode(n)) {}
  INLINE translation(float x, float y, float z, const ref<node> &n) :
    translation(vec3f(x,y,z), n) {}
  vec3f p;
  ref<node> n;
};
struct rotation : node {
  INLINE rotation(const quat3f &q, const ref<node> &n) :
    node(C_ROTATION, aabb::all()), q(q),
    n(fixednode(n)) {}
  INLINE rotation(float deg0, float deg1, float deg2, const ref<node> &n) :
    rotation(quat3f(deg2rad(deg0),deg2rad(deg1),deg2rad(deg2)),n) {}
  quat3f q;
  ref<node> n;
};

void distr(const node *n, const vec3f * RESTRICT pos, const float * RESTRICT normaldist,
           float * RESTRICT dist, u32 * RESTRICT matindex, int num,
           const aabb &box)
{
  switch (n->type) {
    case C_UNION: {
      const auto u = static_cast<const U*>(n);
      const auto isecleft = intersection(u->left->box, box);
      const auto isecright = intersection(u->right->box, box);
      const auto goleft = all(le(isecleft.pmin, isecleft.pmax));
      const auto goright = all(le(isecright.pmin, isecright.pmax));
      if (goleft && goright) {
        distr(u->left, pos, normaldist, dist, matindex, num, box);
        float tempdist[64];
        u32 tempmatindex[64];
        loopi(num) {
          tempdist[i] = FLT_MAX;
          tempmatindex[i] = MAT_AIR_INDEX;
        }
        distr(u->right, pos, normaldist, tempdist, tempmatindex, num, box);
        loopi(num) matindex[i] = max(matindex[i], tempmatindex[i]);
        if (normaldist)
          loopi(num)
            dist[i] = abs(tempdist[i]) < normaldist[i] ? tempdist[i] : min(dist[i], tempdist[i]);
        else
          loopi(num) dist[i] = min(dist[i], tempdist[i]);
      } else if (goleft)
        distr(u->left, pos, normaldist, dist, matindex, num, box);
      else if (goright)
        distr(u->right, pos, normaldist, dist, matindex, num, box);
    }
    break;
    case C_REPLACE: {
      const auto r = static_cast<const R*>(n);
      const auto isecleft = intersection(r->left->box, box);
      const auto goleft = all(le(isecleft.pmin, isecleft.pmax));
      if (!goleft) return;
      distr(r->left, pos, normaldist, dist, matindex, num, box);
      const auto isecright = intersection(r->right->box, box);
      const auto goright = all(le(isecright.pmin, isecright.pmax));
      if (!goright) return;
      float tempdist[64];
      u32 tempmatindex[64];
      loopi(num) {
        tempdist[i] = FLT_MAX;
        tempmatindex[i] = MAT_AIR_INDEX;
      }
      distr(r->right, pos, normaldist, tempdist, tempmatindex, num, box);
      loopi(num) {
        const auto insideright = tempdist[i] < 0.f && dist[i] < 0.f;
        matindex[i] = insideright ? tempmatindex[i] : matindex[i];
      }
      if (normaldist)
        loopi(num)
          dist[i] = dist[i] < 0.f && abs(tempdist[i]) < normaldist[i] ?
            tempdist[i] : dist[i];
    }
    break;
    case C_INTERSECTION: {
      const auto in = static_cast<const I*>(n);
      const auto isecleft = intersection(in->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      const auto isecright = intersection(in->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      distr(in->left, pos, normaldist, dist, matindex, num, box);
      float tempdist[64];
      loopi(num) tempdist[i] = FLT_MAX;
      distr(in->right, pos, normaldist, tempdist, matindex, num, box);
      loopi(num) {
        dist[i] = max(dist[i], tempdist[i]);
        matindex[i] = dist[i] >= 0.f ? MAT_AIR_INDEX : matindex[i];
      }
    }
    break;
    case C_DIFFERENCE: {
      const auto d = static_cast<const D*>(n);
      const auto isecleft = intersection(d->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      distr(d->left, pos, normaldist, dist, matindex, num, box);

      const auto isecright = intersection(d->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      float tempdist[64];
      u32 tempmatindex[64];
      loopi(num) tempdist[i] = FLT_MAX;
      distr(d->right, pos, normaldist, tempdist, tempmatindex, num, box);
      loopi(num) {
        dist[i] = max(dist[i], -tempdist[i]);
        matindex[i] = dist[i] >= 0.f ? MAT_AIR_INDEX : matindex[i];
      }
    }
    break;
    case C_TRANSLATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto t = static_cast<const translation*>(n);
      vec3f tpos[64];
      loopi(num) tpos[i] = pos[i] - t->p;
      const aabb tbox(box.pmin-t->p, box.pmax-t->p);
      distr(t->n, tpos, normaldist, dist, matindex, num, tbox);
    }
    break;
    case C_ROTATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto r = static_cast<const rotation*>(n);
      vec3f tpos[64];
      loopi(num) tpos[i] = xfmpoint(conj(r->q), pos[i]);
      distr(r->n, tpos, normaldist, dist, matindex, num, aabb::all());
    }
    break;
    case C_PLANE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto p = static_cast<const plane*>(n);
      loopi(num) {
        dist[i] = dot(pos[i], p->p.xyz()) + p->p.w;
        matindex[i] = dist[i] < 0.f ? p->matindex : matindex[i];
      }
    }
    break;

#define CYL(NAME, COORD)\
  case C_CYLINDER##NAME: {\
    const auto isec = intersection(n->box, box);\
    if (any(gt(isec.pmin, isec.pmax))) break;\
    const auto c = static_cast<const cylinder##COORD*>(n);\
    loopi(num) {\
      dist[i] = length(pos[i].COORD()-c->c##COORD) - c->r;\
      matindex[i] = dist[i] < 0.f ? c->matindex : matindex[i];\
    }\
  }\
  break;
  CYL(XY,xy); CYL(XZ,xz); CYL(YZ,yz);
#undef CYL

    case C_SPHERE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto s = static_cast<const sphere*>(n);
      loopi(num) {
        dist[i] = length(pos[i]) - s->r;
        matindex[i] = dist[i] < 0.f ? s->matindex : matindex[i];
      }
    }
    break;
    case C_BOX: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto b = static_cast<const struct box*>(n);
      const auto extent = b->extent;
      loopi(num) {
        const auto pd = abs(pos[i])-extent;
        dist[i] = min(max(pd.x,max(pd.y,pd.z)),0.0f) + length(max(pd,vec3f(zero)));
        matindex[i] = dist[i] < 0.f ? b->matindex : matindex[i];
      }
    }
    break;
    case C_EMPTY: break;
    case C_INVALID: assert("unreachable" && false);
  }
}

void dist(const node *n, const vec3f *pos, const float *normaldist,
          float *d, u32 *mat, int num, const aabb &box)
{
  assert(num <= 64);
  loopi(num) d[i] = FLT_MAX;
  loopi(num) mat[i] = MAT_AIR_INDEX;
  distr(n, pos, normaldist, d, mat, num, box);
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
    case C_REPLACE: {
      const auto r = static_cast<const R*>(n);
      return dist(r->left, pos, box);
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
    case C_EMPTY: return FLT_MAX;
    default: assert("unreachable" && false); return FLT_MAX;
  }
}

static ref<node> root;
static void setroot(const ref<node> &node) {root = node;}
node *makescene() {
  return root.ptr;
}

void destroyscene(node *n) {
  assert(n == root.ptr);
  root = NULL;
}

void start() {
#define ENUM(NAMESPACE,NAME,VALUE)\
  static const u32 NAME = VALUE;\
  luabridge::getGlobalNamespace(script::luastate())\
  .beginNamespace(#NAMESPACE)\
    .addVariable(#NAME, const_cast<u32*>(&NAME), false)\
  .endNamespace()
  ENUM(csg, mat_air_index, MAT_AIR_INDEX);
  ENUM(csg, mat_simple_index, MAT_SIMPLE_INDEX);
  ENUM(csg, mat_snoise_index, MAT_SNOISE_INDEX);
#undef ENUM

#define ADDCLASS(NAME, CONSTRUCTOR) \
  .deriveClass<NAME,node>(#NAME)\
    .addConstructor<CONSTRUCTOR, ref<NAME>>()\
  .endClass()
  luabridge::getGlobalNamespace(script::luastate())
  .beginNamespace("csg")
    .addFunction("setroot", setroot)
    .beginClass<node>("node")
      .addFunction("setmin", &node::setmin)
      .addFunction("setmax", &node::setmax)
    .endClass()
    ADDCLASS(U,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(D,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(I,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(R,void(*)(const ref<node>&, const ref<node>&))
    ADDCLASS(box,void(*)(float,float,float,u32))
    ADDCLASS(plane,void(*)(float,float,float,float,u32))
    ADDCLASS(sphere,void(*)(float,u32))
    ADDCLASS(cylinderxz,void(*)(float,float,float,u32))
    ADDCLASS(cylinderxy,void(*)(float,float,float,u32))
    ADDCLASS(cylinderyz,void(*)(float,float,float,u32))
    ADDCLASS(translation,void(*)(float,float,float,const ref<node>&))
    ADDCLASS(rotation,void(*)(float,float,float,const ref<node>&))
  .endNamespace();
#undef ADDCLASS
}
void finish() {}
} /* namespace csg */
} /* namespace q */

