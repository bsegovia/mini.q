/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - nath.cpp -> implements heavy math routines
 -------------------------------------------------------------------------*/
#include "math.hpp"
namespace q {
template <typename T>
mat4x4<T> mat4x4<T>::inverse(void) const {
  mat4x4<T> inv;
  inv.vx.x = vy.y*vz.z*vw.w - vy.y*vz.w*vw.z - vz.y*vy.z*vw.w
           + vz.y*vy.w*vw.z + vw.y*vy.z*vz.w - vw.y*vy.w*vz.z;
  inv.vy.x = -vy.x*vz.z*vw.w + vy.x*vz.w*vw.z + vz.x*vy.z*vw.w
           - vz.x*vy.w*vw.z - vw.x*vy.z*vz.w + vw.x*vy.w*vz.z;
  inv.vz.x = vy.x*vz.y*vw.w - vy.x*vz.w*vw.y - vz.x*vy.y*vw.w
           + vz.x*vy.w*vw.y + vw.x*vy.y*vz.w - vw.x*vy.w*vz.y;
  inv.vw.x = -vy.x*vz.y*vw.z + vy.x*vz.z*vw.y + vz.x*vy.y*vw.z
           - vz.x*vy.z*vw.y - vw.x*vy.y*vz.z + vw.x*vy.z*vz.y;
  inv.vx.y = -vx.y*vz.z*vw.w + vx.y*vz.w*vw.z + vz.y*vx.z*vw.w
           - vz.y*vx.w*vw.z - vw.y*vx.z*vz.w + vw.y*vx.w*vz.z;
  inv.vy.y = vx.x*vz.z*vw.w - vx.x*vz.w*vw.z - vz.x*vx.z*vw.w
           + vz.x*vx.w*vw.z + vw.x*vx.z*vz.w - vw.x*vx.w*vz.z;
  inv.vz.y = -vx.x*vz.y*vw.w + vx.x*vz.w*vw.y + vz.x*vx.y*vw.w
           - vz.x*vx.w*vw.y - vw.x*vx.y*vz.w + vw.x*vx.w*vz.y;
  inv.vw.y = vx.x*vz.y*vw.z - vx.x*vz.z*vw.y - vz.x*vx.y*vw.z
           + vz.x*vx.z*vw.y + vw.x*vx.y*vz.z - vw.x*vx.z*vz.y;
  inv.vx.z = vx.y*vy.z*vw.w - vx.y*vy.w*vw.z - vy.y*vx.z*vw.w
           + vy.y*vx.w*vw.z + vw.y*vx.z*vy.w - vw.y*vx.w*vy.z;
  inv.vy.z = -vx.x*vy.z*vw.w + vx.x*vy.w*vw.z + vy.x*vx.z*vw.w
           - vy.x*vx.w*vw.z - vw.x*vx.z*vy.w + vw.x*vx.w*vy.z;
  inv.vz.z = vx.x*vy.y*vw.w - vx.x*vy.w*vw.y - vy.x*vx.y*vw.w
           + vy.x*vx.w*vw.y + vw.x*vx.y*vy.w - vw.x*vx.w*vy.y;
  inv.vw.z = -vx.x*vy.y*vw.z + vx.x*vy.z*vw.y + vy.x*vx.y*vw.z
           - vy.x*vx.z*vw.y - vw.x*vx.y*vy.z + vw.x*vx.z*vy.y;
  inv.vx.w = -vx.y*vy.z*vz.w + vx.y*vy.w*vz.z + vy.y*vx.z*vz.w
           - vy.y*vx.w*vz.z - vz.y*vx.z*vy.w + vz.y*vx.w*vy.z;
  inv.vy.w = vx.x*vy.z*vz.w - vx.x*vy.w*vz.z - vy.x*vx.z*vz.w
           + vy.x*vx.w*vz.z + vz.x*vx.z*vy.w - vz.x*vx.w*vy.z;
  inv.vz.w = -vx.x*vy.y*vz.w + vx.x*vy.w*vz.y + vy.x*vx.y*vz.w
           - vy.x*vx.w*vz.y - vz.x*vx.y*vy.w + vz.x*vx.w*vy.y;
  inv.vw.w = vx.x*vy.y*vz.z - vx.x*vy.z*vz.y - vy.x*vx.y*vz.z
           + vy.x*vx.z*vz.y + vz.x*vx.y*vy.z - vz.x*vx.z*vy.y;
  return inv / (vx.x*inv.vx.x + vx.y*inv.vy.x + vx.z*inv.vz.x + vx.w*inv.vw.x);
}
template mat4x4<float> mat4x4<float>::inverse(void) const;
} /* namespace q */

