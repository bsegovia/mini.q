/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - csgscalar.cpp -> implements csg evaluation in plain C (no simd)
 -------------------------------------------------------------------------*/
#include "csg.hpp"
#include "csginternal.hpp"

namespace q {
namespace csg {
static void distr(const node *RESTRICT n, const array3f &RESTRICT pos,
                  const arrayf *RESTRICT normaldist, arrayf &RESTRICT dist,
                  arrayi &RESTRICT matindex, int num, const aabb &RESTRICT box)
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
        arrayf tempdist;
        arrayi tempmatindex;
        loopi(num) {
          tempdist[i] = FLT_MAX;
          tempmatindex[i] = MAT_AIR_INDEX;
        }
        distr(u->right, pos, normaldist, tempdist, tempmatindex, num, box);
        loopi(num) matindex[i] = max(matindex[i], tempmatindex[i]);
        if (normaldist)
          loopi(num)
            dist[i] = abs(tempdist[i]) < (*normaldist)[i] ?
              tempdist[i] :
              min(dist[i], tempdist[i]);
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
      arrayf tempdist;
      arrayi tempmatindex;
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
          dist[i] = dist[i] < 0.f && abs(tempdist[i]) < (*normaldist)[i] ?
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
      arrayf tempdist;
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
      arrayf tempdist;
      arrayi tempmatindex;
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
      array3f tpos;
      loopi(num) set(tpos, get(pos,i) - t->p, i);
      const aabb tbox(box.pmin-t->p, box.pmax-t->p);
      distr(t->n, tpos, normaldist, dist, matindex, num, tbox);
    }
    break;
    case C_ROTATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto r = static_cast<const rotation*>(n);
      array3f tpos;
      loopi(num) set(tpos, xfmpoint(conj(r->q), get(pos,i)), i);
      distr(r->n, tpos, normaldist, dist, matindex, num, aabb::all());
    }
    break;
    case C_PLANE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto p = static_cast<const plane*>(n);
      loopi(num) {
        dist[i] = dot(get(pos,i), p->p.xyz()) + p->p.w;
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
      dist[i] = length(get(pos,i).COORD()-c->c##COORD) - c->r;\
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
        dist[i] = length(get(pos,i)) - s->r;
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
        const auto pd = abs(get(pos,i))-extent;
        dist[i] = min(max(pd.x,max(pd.y,pd.z)),0.0f) + length(max(pd,vec3f(zero)));
        matindex[i] = dist[i] < 0.f ? b->matindex : matindex[i];
      }
    }
    break;
    case C_EMPTY: break;
    case C_INVALID: assert("unreachable" && false);
  }
}

void dist(const node *RESTRICT n, const array3f &RESTRICT pos,
          const arrayf *RESTRICT normaldist, arrayf &RESTRICT d,
          arrayi &RESTRICT mat, int num, const aabb &RESTRICT box)
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
} /* namespace csg */
} /* namespace q */

