/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rt.cpp -> exposes ray tracing kernels
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"

namespace q {
namespace rt {
struct ray {
  INLINE ray(void) {}
  INLINE ray(vec3f org, vec3f dir, float near = 0.f, float far = FLT_MAX)
    : org(org), dir(dir), tnear(near), tfar(far) {}
  vec3f org, dir;
  float tnear, tfar;
};

struct camera {
  camera(vec3f org, vec3f up, vec3f view, float fov, float ratio);
  INLINE ray generate(int w, int h, int x, int y) const {
    const float rw = rcp(float(w)), rh = rcp(float(h));
    const vec3f sxaxis = xaxis*rw, szaxis = zaxis*rh;
    const vec3f dir = imgplaneorg + float(x)*sxaxis + float(y)*szaxis;
    return ray(org, dir);
  }
  vec3f org, up, view, imgplaneorg, xaxis, zaxis;
  float fov, ratio, dist;
};

enum { TILESIZE = 16 };

void buildbvh(vec3f *v, u32 *idx, u32 idxnum);
void raytrace(const char *bmp, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect);
} /* namespace rt */
} /* namespace q */
