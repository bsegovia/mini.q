/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rt.cpp -> implements ray tracing kernels
 -------------------------------------------------------------------------*/
#include "bvh.hpp"
#include "rt.hpp"
#include "rtscalar.hpp"
#include "rtsse.hpp"
#include "rtavx.hpp"
#include "base/math.hpp"
#include "base/console.hpp"
#include "base/task.hpp"

namespace q {
namespace rt {
static intersector *world = NULL;

// create a triangle soup and make a mesh out of it
void buildbvh(vec3f *v, u32 *idx, u32 idxnum) {
  const auto start = sys::millis();
  const auto trinum = idxnum/3;
  const auto prim = NEWAE(primitive, trinum);
  loopi(trinum) {
    loopj(3) prim[i].v[j] = v[idx[3*i+j]];
    prim[i].type = primitive::TRI;
  }
  world = create(prim, trinum);
  const auto ms = sys::millis() - start;
  con::out("bvh: elapsed %f ms", float(ms));
}

camera::camera(vec3f org, vec3f up, vec3f view, float fov, float ratio) :
  org(org), up(up), view(view), fov(fov), ratio(ratio)
{
  const float left = -ratio * 0.5f;
  const float top = 0.5f;
  dist = 0.5f / tan(fov * float(pi) / 360.f);
  view = normalize(view);
  up = normalize(up);
  xaxis = cross(view, up);
  xaxis = normalize(xaxis);
  zaxis = cross(view, xaxis);
  zaxis = normalize(zaxis);
  imgplaneorg = dist*view + left*xaxis - top*zaxis;
  xaxis *= ratio;
}

#define NORMAL_ONLY 0

//static const vec3f lpos(0.f, -4.f, 2.f);
static const vec3f lpos(0.f, 4.f, 0.f);
static atomic totalraynum;
struct raycasttask : public task {
  raycasttask(intersector *bvhisec, const camera &cam, int *pixels, vec2i dim, vec2i tile) :
    task("raycasttask", tile.x*tile.y, 1, 0, UNFAIR),
    bvhisec(bvhisec), cam(cam), pixels(pixels), dim(dim), tile(tile)
  {}
  INLINE void primarypoint(vec2i tileorg, array3f &pos, array3f &nor, arrayi &mask) {
    raypacket p;
    packethit hit;
    avx::visibilitypacket(cam, p, tileorg, dim);
    avx::clearpackethit(hit);
    avx::closest(*bvhisec, p, hit);
    avx::primarypoint(p, hit, pos, nor, mask);
  }
  virtual void run(u32 tileID) {
    const vec2i tilexy(tileID%tile.x, tileID/tile.x);
    const vec2i tileorg = int(TILESIZE) * tilexy;

#if NORMAL_ONLY
    // primary intersections
    raypacket p;
    packethit hit;
    avx::visibilitypacket(cam, p, tileorg, dim);
    avx::clearpackethit(hit);
    avx::closest(*bvhisec, p, hit);
    avx::writenormal(hit, tileorg, dim, pixels);
    totalraynum += TILESIZE*TILESIZE;
#else
    // shadow rays toward the light source
    array3f pos, nor;
    arrayi mask;
    raypacket shadow;
    packetshadow occluded;
    primarypoint(tileorg, pos, nor, mask);
    avx::shadowpacket(pos, mask, lpos, shadow, occluded, TILESIZE*TILESIZE);
    avx::occluded(*bvhisec, shadow, occluded);
    avx::writendotl(shadow, nor, occluded, tileorg, dim, pixels);
    totalraynum += shadow.raynum+TILESIZE*TILESIZE;
#endif
  }
  intersector *bvhisec;
  const camera &cam;
  int *pixels;
  vec2i dim;
  vec2i tile;
};
static int *pixels=NULL;
void raytrace(const char *bmp, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect)
{
  const mat3x3f r = mat3x3f::rotate(-ypr.x,vec3f(0.f,1.f,0.f))*
                    mat3x3f::rotate(-ypr.y,vec3f(1.f,0.f,0.f))*
                    mat3x3f::rotate(-ypr.z,vec3f(0.f,0.f,1.f));
  const camera cam(pos, -r.vy, -r.vz, fovy, aspect);
  const vec2i dim(w,h), tile(dim/int(TILESIZE));
  if (pixels==NULL)
  pixels = (int*)MALLOC(w*h*sizeof(int));
  const auto start = sys::millis();
  totalraynum=0;
  ref<task> isectask = NEW(raycasttask, world, cam, pixels, dim, tile);
  isectask->scheduled();
  isectask->wait();
  const auto duration = float(sys::millis()-start);
  con::out("rt: %i ms, %f Mray/s", int(duration), 1000.f*(float(totalraynum)*1e-6f)/duration);
  sys::writebmp(pixels, w, h, bmp);
  //FREE(pixels);
}
} /* namespace rt */
} /* namespace q */

