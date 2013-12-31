/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - iso.hpp -> implements common functions for iso surface tesselation, collision
 - ...etc
 -------------------------------------------------------------------------*/
#include "iso.hpp"

namespace q {
namespace iso {

mesh::~mesh() {
  if (m_pos) FREE(m_pos);
  if (m_nor) FREE(m_nor);
  if (m_index) FREE(m_index);
}
vec3f gradient(distance_field d, const vec3f &pos, float grad_step) {
  const vec3f dx = vec3f(grad_step, 0.f, 0.f);
  const vec3f dy = vec3f(0.f, grad_step, 0.f);
  const vec3f dz = vec3f(0.f, 0.f, grad_step);
  return normalize(vec3f(d(pos+dx)-d(pos-dx), d(pos+dy)-d(pos-dy), d(pos+dz)-d(pos-dz)));
}
} /* namespace iso */
} /* namespace q */

