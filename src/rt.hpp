/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rt.cpp -> exposes ray tracing kernels
 -------------------------------------------------------------------------*/
#include "base/math.hpp"

namespace q {
namespace rt {
void buildbvh(vec3f *v, u32 *idx, u32 idxnum);
void raytrace(const char *bmp, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect);
} /* namespace rt */
} /* namespace q */
