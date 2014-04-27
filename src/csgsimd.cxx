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
struct ssebox {
  ssebox(const ssef &pmin, const ssef &pmax) : pmin(pmin), pmax(pmax) {}
  ssebox(const aabb &box) {
    pmin = ssef::loadu(&box.pmin);
    pmax = ssef::loadu(&box.pmax);
  }
  ssef pmin,pmax;
};

INLINE ssebox intersection(const ssebox &b0, const ssebox &b1) {
  return ssebox(max(b0.pmin,b1.pmin), min(b0.pmax,b1.pmax));
}
INLINE bool empty(const ssebox &box) {
  return (movemask(box.pmin>box.pmax)&0x7) != 0;
}

#if 1
static void distr(const node *RESTRICT n, const array3f &RESTRICT pos,
                  const arrayf *RESTRICT normaldist, arrayf &RESTRICT dist,
                  arrayi &RESTRICT matindex, int packetnum,
                  const ssebox & RESTRICT box)
{
  switch (n->type) {
    case C_UNION: {
      const auto u = static_cast<const U*>(n);
      const auto isecleft = intersection(ssebox(u->left->box), box);
      const auto isecright = intersection(ssebox(u->right->box), box);
      const auto goleft = !empty(isecleft);
      const auto goright = !empty(isecright);
      if (goleft && goright) {
        distr(u->left, pos, normaldist, dist, matindex, packetnum, box);
        CACHE_LINE_ALIGNED arrayf tempdist;
        CACHE_LINE_ALIGNED arrayi tempmatindex;
        loopi(packetnum) {
          const auto idx = i*soaf::size;
          store(&tempdist[idx], soaf(FLT_MAX));
          store(&tempmatindex[idx], soai(MAT_AIR_INDEX));
        }
        distr(u->right, pos, normaldist, tempdist, tempmatindex, packetnum, box);
        loopi(packetnum) {
          const auto idx = i*soaf::size;
          const auto old = soai::load(&matindex[idx]);
          const auto tmp = soai::load(&tempmatindex[idx]);
          const auto newindex = select(old > tmp, old, tmp);
          store(&matindex[idx], newindex);
        }
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
      const auto isecleft = intersection(ssebox(r->left->box), box);
      const auto goleft = !empty(isecleft);
      if (!goleft) return;
      distr(r->left, pos, normaldist, dist, matindex, packetnum, box);
      const auto isecright = intersection(ssebox(r->right->box), box);
      const auto goright = !empty(isecright);
      if (!goright) return;
      CACHE_LINE_ALIGNED arrayf tempdist;
      CACHE_LINE_ALIGNED arrayi tempmatindex;
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
      const auto isecleft = intersection(ssebox(in->left->box), box);
      if (empty(isecleft)) return;
      const auto isecright = intersection(ssebox(in->right->box), box);
      if (empty(isecright)) return;
      distr(in->left, pos, normaldist, dist, matindex, packetnum, box);
      CACHE_LINE_ALIGNED arrayf tempdist;
      loopi(packetnum) store(&tempdist[i*soaf::size], soaf(FLT_MAX));
      distr(in->right, pos, normaldist, tempdist, matindex, packetnum, box);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto d = soaf::load(&dist[idx]);
        const auto td = soaf::load(&tempdist[idx]);
        const auto md = max(d,td);
        const auto oldindex = soai::load(&matindex[idx]);
        const auto airindex = soai(MAT_AIR_INDEX);
        const auto newindex = select(md>=soaf(zero), airindex, oldindex);
        store(&dist[idx], md);
        store(&matindex[idx], newindex);
      }
    }
    break;
    case C_DIFFERENCE: {
      const auto d = static_cast<const D*>(n);
      const auto isecleft = intersection(ssebox(d->left->box), box);
      if (empty(isecleft)) break;
      distr(d->left, pos, normaldist, dist, matindex, packetnum, box);

      const auto isecright = intersection(ssebox(d->right->box), box);
      if (empty(isecright)) break;
      CACHE_LINE_ALIGNED arrayf tempdist;
      CACHE_LINE_ALIGNED arrayi tempmatindex;
      loopi(packetnum) store(&tempdist[i*soaf::size], soaf(FLT_MAX));
      distr(d->right, pos, normaldist, tempdist, tempmatindex, packetnum, box);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto d = soaf::load(&dist[idx]);
        const auto td = soaf::load(&tempdist[idx]);
        const auto md = max(d,-td);
        const auto oldindex = soai::load(&matindex[idx]);
        const auto airindex = soai(MAT_AIR_INDEX);
        const auto newindex = select(md>=soaf(zero), airindex, oldindex);
        store(&dist[idx], md);
        store(&matindex[idx], newindex);
      }
    }
    break;
    case C_TRANSLATION: {
      const auto isec = intersection(ssebox(n->box), box);
      if (empty(isec)) break;
      const auto t = static_cast<const translation*>(n);
      const auto tp = soa3f(t->p);
      CACHE_LINE_ALIGNED array3f tpos;
      loopi(packetnum) sset(tpos, sget(pos,i) - tp, i);
      const auto ssep = ssef::loadu(&t->p);
      const ssebox tbox(box.pmin-ssep, box.pmax-ssep);
      distr(t->n, tpos, normaldist, dist, matindex, packetnum, tbox);
    }
    break;
    case C_ROTATION: {
      const auto isec = intersection(ssebox(n->box), box);
      if (empty(isec)) break;
      const auto r = static_cast<const rotation*>(n);
      const auto rq = quat<soaf>(conj(r->q));
      CACHE_LINE_ALIGNED array3f tpos;
      loopi(packetnum) sset(tpos, xfmpoint(rq, sget(pos,i)), i);
      distr(r->n, tpos, normaldist, dist, matindex, packetnum, aabb::all());
    }
    break;
    case C_PLANE: {
      const auto isec = intersection(ssebox(n->box), box);
      if (empty(isec)) break;
      const auto p = static_cast<const plane*>(n);
      const auto pp = soa3f(p->p.xyz());
      const auto d = soaf(p->p.w);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto nd = dot(sget(pos,i), pp) + d;
        const auto oldindex = soai::load(&matindex[idx]);
        const auto newindex = soai(p->matindex);
        const auto m = nd<soaf(zero);
        store(&dist[idx], nd);
        store(&matindex[idx], select(m, newindex, oldindex));
      }
    }
    break;

#define CYL(NAME, COORD)\
  case C_CYLINDER##NAME: {\
    const auto isec = intersection(ssebox(n->box), box);\
    if (empty(isec)) break;\
    const auto c = static_cast<const cylinder##COORD*>(n);\
    const auto cc = soa2f(c->c##COORD);\
    const auto r = soaf(c->r);\
    loopi(packetnum) {\
      const auto idx = i*soaf::size;\
      const auto p = sget(pos,i).COORD();\
      const auto nd = length(p - cc) - r;\
      const auto oldindex = soai::load(&matindex[idx]);\
      const auto newindex = soai(c->matindex);\
      const auto m = nd<soaf(zero);\
      store(&dist[idx], nd);\
      store(&matindex[idx], select(m, newindex, oldindex));\
    }\
  }\
  break;
  CYL(XY,xy); CYL(XZ,xz); CYL(YZ,yz);
#undef CYL

    case C_SPHERE: {
      const auto isec = intersection(ssebox(n->box), box);
      if (empty(isec)) break;
      const auto s = static_cast<const sphere*>(n);
      const auto r = soaf(s->r);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto oldindex = soai::load(&matindex[idx]);
        const auto newindex = soai(s->matindex);
        const auto nd = length(sget(pos,i)) - r;
        const auto m = nd<soaf(zero);
        store(&dist[idx], nd);
        store(&matindex[idx], select(m, newindex, oldindex));
      }
    }
    break;
    case C_BOX: {
      const auto isec = intersection(ssebox(n->box), box);
      if (empty(isec)) break;
      const auto b = static_cast<const struct box*>(n);
      const auto extent = soa3f(b->extent);
      loopi(packetnum) {
        const auto idx = i*soaf::size;
        const auto oldindex = soai::load(&matindex[idx]);
        const auto newindex = soai(b->matindex);
        const auto pd = abs(sget(pos,i))-extent;
        const auto nd = min(max(pd.x,max(pd.y,pd.z)),soaf(zero)) + length(max(pd,soa3f(zero)));
        const auto m = nd<soaf(zero);
        store(&dist[idx], nd);
        store(&matindex[idx], select(m, newindex, oldindex));
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
  distr(n, pos, normaldist, d, mat, packetnum, ssebox(box));
  AVX_ZERO_UPPER();
}
#endif
} /* namespace NAMESPACE */
} /* namespace rt */
} /* namespace q */

