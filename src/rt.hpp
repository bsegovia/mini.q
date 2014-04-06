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

static const float SHADOWRAYBIAS = 1e-2f;
static const u32 MAXRAYNUM = 256u;
typedef CACHE_LINE_ALIGNED array<int,MAXRAYNUM> arrayi;
typedef CACHE_LINE_ALIGNED array<float,MAXRAYNUM> arrayf;
typedef CACHE_LINE_ALIGNED arrayf array2f[2];
typedef CACHE_LINE_ALIGNED arrayf array3f[3];
typedef CACHE_LINE_ALIGNED arrayf array4f[4];

struct CACHE_LINE_ALIGNED raypacket {
  static const u32 SHAREDORG     = 1<<0;
  static const u32 SHAREDDIR     = 1<<1;
  static const u32 INTERVALARITH = 1<<2;
  static const u32 CORNERRAYS    = 1<<3;
  INLINE raypacket(void) : raynum(0), flags(0) {}
  INLINE void setorg(vec3f org, u32 rayid) {
    vorg[0][rayid] = org.x;
    vorg[1][rayid] = org.y;
    vorg[2][rayid] = org.z;
  }
  INLINE void setdir(vec3f dir, u32 rayid) {
    vdir[0][rayid] = dir.x;
    vdir[1][rayid] = dir.y;
    vdir[2][rayid] = dir.z;
  }
  INLINE vec3f org(u32 rayid=0) const {
    return vec3f(vorg[0][rayid], vorg[1][rayid], vorg[2][rayid]);
  }
  INLINE vec3f dir(u32 rayid=0) const {
    return vec3f(vdir[0][rayid], vdir[1][rayid], vdir[2][rayid]);
  }
  array3f vorg;     // only used when SHAREDORG is *not* set
  array3f vdir;     // only used when SHAREDDIR is *not* set
  ALIGNED(16) vec3f sharedorg;  // only used when SHAREDORG is set
  ALIGNED(16) vec3f shareddir;  // only used when SHAREDDIR is set
  ALIGNED(16) float crx[4]; // only used when CORNERRAYS is set
  ALIGNED(16) float cry[4]; // only used when CORNERRAYS is set
  ALIGNED(16) float crz[4]; // only used when CORNERRAYS is set
  u32 raynum;       // number of rays active in the packet
  u32 flags;        // exposes property of the packet
};

struct CACHE_LINE_ALIGNED packethit : noncopyable {
  arrayf t, u, v;
  array3f n;
  arrayi id;
  INLINE bool ishit(u32 idx) const { return id[idx] != -1; }
  INLINE vec3f getnormal(u32 idx) const { return vec3f(n[0][idx],n[1][idx],n[2][idx]); }
};

struct CACHE_LINE_ALIGNED packetshadow : noncopyable {
  arrayf t;
  arrayi occluded, mapping;
};

struct CACHE_LINE_ALIGNED packedprimary : noncopyable {
  array3f pos, nor;
  arrayi mapping;
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

