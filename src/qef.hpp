/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - qef.hpp -> exposes quadratic error function and matrices
 - original code for SVD by Ronen Tzur (rtzur@shani.net)
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"
#include "base/pair.hpp"

namespace q {
namespace qef {
// QEF, a class implementing the quadric error function
//      E[x] = P - Ni . Pi
// Given at least three points Pi, each with its respective normal vector Ni,
// that describe at least two planes, the QEF evalulates to the point x.
vec3f evaluate(double mat[][3], double *vec, int rows);

/*-------------------------------------------------------------------------
 - quadratic error matrix (as proposed by Garland et al.)
 -------------------------------------------------------------------------*/
struct qem {
  INLINE qem() {ZERO(this); next=-1;}
  INLINE qem(vec3f v0, vec3f v1, vec3f v2) : timestamp(0), next(-1) {
    init(cross(v0-v1,v0-v2), v0);
  }
  INLINE qem(vec3f d, vec3f p) : timestamp(0), next(-1) { init(d,p); }
  INLINE void init(const vec3f &d, const vec3f &p) {
    const auto len = double(length(d));
    if (len != 0.) {
      // slightly bias the error to make the error function returning very small
      // negative value. this will be cleanly clamped to zero. zero errors are
      // useful since they will map to fully flat surface that we can then
      // handle specifically.
      const auto dp = vec3d(p);
      const auto n = vec3d(d)/len;
      const auto d = -dot(n, dp);
      const auto a = n.x, b = n.y, c = n.z;

      // weight the qem with the triangle area given by the length of
      // unormalized normal vector
      aa = a*a*len; bb = b*b*len; cc = c*c*len; dd = d*d*len;
      ab = a*b*len; ac = a*c*len; ad = a*d*len;
      bc = b*c*len; bd = b*d*len;
      cd = c*d*len;
    } else
      init(zero);
  }
  INLINE void init(zerotype) {
    aa = bb = cc = dd = 0.0;
    ab = ac = ad = 0.0;
    bc = bd = 0.0;
    cd = 0.0;
  }
  INLINE qem operator+= (const qem &q) {
    aa+=q.aa; bb+=q.bb; cc+=q.cc; dd+=q.dd;
    ab+=q.ab; ac+=q.ac; ad+=q.ad;
    bc+=q.bc; bd+=q.bd;
    cd+=q.cd;
    return *this;
  }
  double error(const vec3f &v, double minerror) const {
    const auto x = double(v.x), y = double(v.y), z = double(v.z);
    const auto err = x*x*aa + 2.0*x*y*ab + 2.0*x*z*ac + 2.0*x*ad +
                     y*y*bb + 2.0*y*z*bc + 2.0*y*bd
                            + z*z*cc     + 2.0*z*cd + dd;
    return err < minerror ? 0.0 : err;
  }
  double aa, bb, cc, dd;
  double ab, ac, ad;
  double bc, bd;
  double cd;
  int timestamp, next;
};

INLINE qem operator+ (const qem &q0, const qem &q1) {
  qem q = q0;
  q += q1;
  return q;
}

INLINE pair<double,int>
findbest(const qem &q0, const qem &q1, const vec3f &p0, const vec3f &p1, double minerror) {
  const auto q = q0 + q1;
  const auto error0 = q.error(p0, minerror);
  const auto error1 = q.error(p1, minerror);
  return error0 < error1 ? makepair(error0, 0) : makepair(error1, 1);
}
} /* namespace qef */
} /* namespace q */

