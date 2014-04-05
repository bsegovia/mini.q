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

static const vec3f lpos(0.f, -4.f, 2.f);
static atomic totalraynum;
struct raycasttask : public task {
  raycasttask(intersector *bvhisec, const camera &cam, int *pixels, vec2i dim, vec2i tile) :
    task("raycasttask", tile.x*tile.y, 1, 0, UNFAIR),
    bvhisec(bvhisec), cam(cam), pixels(pixels), dim(dim), tile(tile)
  {}
  virtual void run(u32 tileID) {
    const vec2i tilexy(tileID%tile.x, tileID/tile.x);
    const vec2i tileorg = int(TILESIZE) * tilexy;
    raypacket p;
    packethit hit;
    visibilitypacket(cam, p, tileorg, dim);
    visibilitypackethit(hit);
    avx::closest(*bvhisec, p, hit);

#define NORMAL_ONLY 0
#if !NORMAL_ONLY
    // exclude points that interesect nothing
    int mapping[TILESIZE*TILESIZE], curr = 0;
    raypacket shadow;
    packetshadow occluded;
    loopi(int(p.raynum)) {
      if (hit.ishit(i)) {
        mapping[i] = curr;
        const auto n = normalize(hit.getnormal(i));
        hit.n[0][i] = n.x;
        hit.n[1][i] = n.y;
        hit.n[2][i] = n.z;
        const auto dst = p.sharedorg + hit.t[i] * p.dir(i) + n * 1e-2f;
        const auto dir = dst-lpos;
        shadow.setdir(dir, curr);
        occluded.t[curr] = 1.f;
        occluded.occluded[curr] = 0;
        ++curr;
      } else
        mapping[i] = -1;
    }
    shadow.sharedorg = lpos;
    shadow.raynum = curr;
    shadow.flags = raypacket::SHAREDORG;
    avx::occluded(*bvhisec, shadow, occluded);

    for (u32 y = 0; y < u32(TILESIZE); ++y)
    for (u32 x = 0; x < u32(TILESIZE); ++x) {
      const int offset = (tileorg.x+x)+dim.x*(tileorg.y+y);
      const int idx = x+y*TILESIZE;
      if (hit.ishit(idx)) {
        const auto sid = mapping[idx];
        if (!occluded.occluded[sid]) {
          const auto shade = dot(hit.getnormal(idx), -normalize(shadow.dir(sid)));
          const auto d = int(255.f*min(max(0.f,shade),1.f));
          pixels[offset] = d|(d<<8)|(d<<16)|(0xff<<24);
        } else
          pixels[offset] = 0;
      } else
        pixels[offset] = 0;
    }
    totalraynum += curr+TILESIZE*TILESIZE;
#else
    for (u32 y = 0; y < u32(TILESIZE); ++y)
    for (u32 x = 0; x < u32(TILESIZE); ++x) {
      const int offset = (tileorg.x+x)+dim.x*(tileorg.y+y);
      const int idx = x+y*TILESIZE;
      if (hit.ishit(idx)) {
        const auto n = vec3i(clamp(normalize(hit.getnormal(idx)), vec3f(zero), vec3f(one))*255.f);
        pixels[offset] = n.x|(n.y<<8)|(n.z<<16)|(0xff<<24);
      } else
        pixels[offset] = 0;
    }
    totalraynum += TILESIZE*TILESIZE;
#endif
  }
  intersector *bvhisec;
  const camera &cam;
  int *pixels;
  vec2i dim;
  vec2i tile;
};

void raytrace(const char *bmp, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect)
{
  const mat3x3f r = mat3x3f::rotate(-ypr.x,vec3f(0.f,1.f,0.f))*
                    mat3x3f::rotate(-ypr.y,vec3f(1.f,0.f,0.f))*
                    mat3x3f::rotate(-ypr.z,vec3f(0.f,0.f,1.f));
  const camera cam(pos, -r.vy, -r.vz, fovy, aspect);
  const vec2i dim(w,h), tile(dim/int(TILESIZE));
  const auto pixels = (int*)MALLOC(w*h*sizeof(int));
  const auto start = sys::millis();
  totalraynum=0;
  ref<task> isectask = NEW(raycasttask, world, cam, pixels, dim, tile);
  isectask->scheduled();
  isectask->wait();
  const auto duration = float(sys::millis()-start);
  con::out("rt: %i ms, %f Mray/s", int(duration), 1000.f*(float(totalraynum)*1e-6f)/duration);
  sys::writebmp(pixels, w, h, bmp);
  FREE(pixels);
}
} /* namespace rt */
} /* namespace q */

