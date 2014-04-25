/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - csgsimd.cpp -> implements csg evaluation using SIMD instructions
 -------------------------------------------------------------------------*/
#include "csginternal.hpp"
#include "csg.hpp"
#include "soa.hpp"

namespace q {
namespace csg {
namespace NAMESPACE {
#if 0
static void distr(const node *RESTRICT n, const array3f &RESTRICT pos,
                  const arrayf *RESTRICT normaldist, arrayf &RESTRICT dist,
                  arrayi &RESTRICT matindex, int packetnum,
                  const aabb &RESTRICT box)
{
  switch (n->type) {
    case C_UNION: {
      const auto u = static_cast<const U*>(n);
      const auto isecleft = intersection(u->left->box, box);
      const auto isecright = intersection(u->right->box, box);
      const auto goleft = all(le(isecleft.pmin, isecleft.pmax));
      const auto goright = all(le(isecright.pmin, isecright.pmax));
      if (goleft && goright) {
        distr(u->left, pos, normaldist, dist, matindex, packetnum, box);
        arrayf tempdist;
        arrayi tempmatindex;
        loopi(packetnum) {
          const auto idx = i*soaf::size;
          store(&tempdist[idx], soaf(FLT_MAX));
          store(&tempmatindex[idx], soai(MAT_AIR_INDEX));
        }
        distr(u->right, pos, normaldist, tempdist, tempmatindex, packetnum, box);
        loopi(packetnum) matindex[i] = max(matindex[i], tempmatindex[i]);
        if (normaldist) loopi(packetnum) {
          const auto idx = i*soaf::size;
          const auto d = soaf::load(&dist[idx]);
          const auto td = soaf::load(&tempdist[idx]);
          const auto nd = soaf::load(&(*normaldist)[idx]);
          const auto mtd = min(d, td);
          const auto sd = select(abs(td)<nd, td, mtd);
          store(&dist[idx], sd);
        } else loopi(packetnum) {
          const auto idx = i*soaf::size;
          const auto d = soaf::load(&dist[idx]);
          const auto td = soaf::load(&tempdist[idx]);
          store(&dist[idx], min(d,td));
        }
      } else if (goleft)
        distr(u->left, pos, normaldist, dist, matindex, packetnum, box);
      else if (goright)
        distr(u->right, pos, normaldist, dist, matindex, packetnum, box);
    }
    break;
    case C_REPLACE: {
      const auto r = static_cast<const R*>(n);
      const auto isecleft = intersection(r->left->box, box);
      const auto goleft = all(le(isecleft.pmin, isecleft.pmax));
      if (!goleft) return;
      distr(r->left, pos, normaldist, dist, matindex, packetnum, box);
      const auto isecright = intersection(r->right->box, box);
      const auto goright = all(le(isecright.pmin, isecright.pmax));
      if (!goright) return;
      arrayf tempdist;
      arrayi tempmatindex;
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        store(&tempdist[idx], soaf(FLT_MAX));
        store(&tempmatindex[idx], soai(MAT_AIR_INDEX));
      }
      distr(r->right, pos, normaldist, tempdist, tempmatindex, packetnum, box);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto d = soaf::load(&dist[idx]);
        const auto td = soaf::load(&tempdist[idx]);
        const auto insideright = (td<soaf(zero)) & (d<soaf(zero));
        const auto tmpindex = soai::load(&tempmatindex[idx]);
        const auto oldindex = soai::load(&matindex[idx]);
        const auto newindex = select(insideright, tmpindex, oldindex);
        store(&matindex[idx], newindex);
      }
      if (normaldist) loopi(packetnum) {
          const auto idx = i*soaf::size;
          const auto d = soaf::load(&dist[idx]);
          const auto td = soaf::load(&tempdist[idx]);
          const auto nd = soaf::load(&(*normaldist)[idx]);
          const auto sd = select((d<soaf(zero)) & (abs(td)<nd), td, d);
          store(&dist[idx], sd);
      }
    }
    break;
    case C_INTERSECTION: {
      const auto in = static_cast<const I*>(n);
      const auto isecleft = intersection(in->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      const auto isecright = intersection(in->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      distr(in->left, pos, normaldist, dist, matindex, packetnum, box);
      arrayf tempdist;
      loopi(packetnum) store(&tempdist[i*soaf::size], soaf(FLT_MAX));
      distr(in->right, pos, normaldist, tempdist, matindex, packetnum, box);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto d = soaf::load(&dist[idx]);
        const auto td = soaf::load(&tempdist[idx]);
        const auto md = max(d,td);
        const auto oldindex = soai::load(&matindex[i]);
        const auto airindex = soai(MAT_AIR_INDEX);
        const auto newindex = select(d>=soaf(zero), airindex, oldindex);
        store(&dist[idx], md);
        store(&matindex[idx], newindex);
      }
    }
    break;
    case C_DIFFERENCE: {
      const auto d = static_cast<const D*>(n);
      const auto isecleft = intersection(d->left->box, box);
      if (!all(le(isecleft.pmin, isecleft.pmax))) break;
      distr(d->left, pos, normaldist, dist, matindex, packetnum, box);

      const auto isecright = intersection(d->right->box, box);
      if (!all(le(isecright.pmin, isecright.pmax))) break;
      arrayf tempdist;
      arrayi tempmatindex;
      loopi(packetnum) store(&tempdist[i*soaf::size], soaf(FLT_MAX));
      distr(d->right, pos, normaldist, tempdist, tempmatindex, packetnum, box);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto d = soaf::load(&dist[idx]);
        const auto td = soaf::load(&tempdist[idx]);
        const auto md = max(d,-td);
        const auto oldindex = soai::load(&matindex[i]);
        const auto airindex = soai(MAT_AIR_INDEX);
        const auto newindex = select(d>=soaf(zero), airindex, oldindex);
        store(&dist[idx], md);
        store(&matindex[idx], newindex);
      }
    }
    break;
    case C_TRANSLATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto t = static_cast<const translation*>(n);
      const auto tp = soa3f(t->p);
      array3f tpos;
      loopi(packetnum) set(tpos, get(pos,i) - t->p, i);
      const aabb tbox(box.pmin-t->p, box.pmax-t->p);
      distr(t->n, tpos, normaldist, dist, matindex, packetnum, tbox);
    }
    break;
    case C_ROTATION: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto r = static_cast<const rotation*>(n);
      array3f tpos;
      loopi(packetnum) set(tpos, xfmpoint(conj(r->q), get(pos,i)), i);
      distr(r->n, tpos, normaldist, dist, matindex, packetnum, aabb::all());
    }
    break;
    case C_PLANE: {
      const auto isec = intersection(n->box, box);
      if (any(gt(isec.pmin, isec.pmax))) break;
      const auto p = static_cast<const plane*>(n);
      const auto pp = soa3f(p->p.xyz());
      const auto d = soaf(p->p.w);
      loopi(packetnum) {
        dist[i] = dot(get(pos,i), pp) + d;
        matindex[i] = dist[i] < 0.f ? p->matindex : matindex[i];
      }
    }
    break;

#define CYL(NAME, COORD)\
  case C_CYLINDER##NAME: {\
    const auto isec = intersection(n->box, box);\
    if (any(gt(isec.pmin, isec.pmax))) break;\
    const auto c = static_cast<const cylinder##COORD*>(n);\
    loopi(packetnum) {\
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
      loopi(packetnum) {
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
      loopi(packetnum) {
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
  const auto packetnum = num/soaf::size + (num%soaf::size?1:0);
  loopi(packetnum) {
    store(&d[i*soaf::size], soaf(FLT_MAX));
    store(&mat[i*soaf::size], soai(MAT_AIR_INDEX));
  }
  distr(n, pos, normaldist, d, mat, packetnum, box);
}
#endif
} /* namespace NAMESPACE */
} /* namespace rt */
} /* namespace q */
