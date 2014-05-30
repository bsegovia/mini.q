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
ref<intersector> world;

void setbvh(const ref<intersector> &bvh) { world = bvh; }
// create a triangle soup and make a mesh out of it
void buildbvh(vec3f *v, u32 *idx, u32 idxnum) {
  const auto start = sys::millis();
  const auto trinum = idxnum/3;
  auto prim = NEWAE(primitive, trinum);
  loopi(trinum) {
    loopj(3) prim[i].v[j] = v[idx[3*i+j]];
    prim[i].type = primitive::TRI;
  }
  world = NEW(intersector, prim, trinum);
  const auto ms = sys::millis() - start;
  SAFE_DELA(prim);
  con::out("bvh: elapsed %f ms", float(ms));
}

// all packets functions we dynamically select at start-up time
static void (*rtclosest)(const intersector&, const raypacket&, packethit&);
static void (*rtoccluded)(const intersector&, const raypacket&, packetshadow&);
static void (*rtvisibilitypacket)(const camera &RESTRICT, raypacket &RESTRICT,
                                  const vec2i &RESTRICT, const vec2i &RESTRICT);
static void (*rtshadowpacket)(const array3f &RESTRICT,
                              const arrayi &RESTRICT,
                              const vec3f &RESTRICT,
                              raypacket &RESTRICT,
                              packetshadow &RESTRICT,
                              int);
static u32 (*rtprimarypoint)(const raypacket &RESTRICT,
                             const packethit &RESTRICT,
                             array3f &RESTRICT,
                             array3f &RESTRICT,
                             arrayi &RESTRICT);
static void (*rtclearpackethit)(packethit&);
static void (*rtwritenormal)(const packethit&, const vec2i&, const vec2i&,
                             int *RESTRICT);
static void (*rtwritendotl)(const raypacket&, const array3f&,
                            const packetshadow&, const vec2i&, const vec2i&,
                            int*);
static void (*rtclear)(const vec2i&, const vec2i&, int*);

#define LOAD(NAME) \
  rtclosest = NAME::closest;\
  rtoccluded = NAME::occluded;\
  rtvisibilitypacket = NAME::visibilitypacket;\
  rtshadowpacket = NAME::shadowpacket;\
  rtprimarypoint = NAME::primarypoint;\
  rtclearpackethit = NAME::clearpackethit;\
  rtwritenormal = NAME::writenormal;\
  rtwritendotl = NAME::writendotl;\
  rtclear = NAME::clear;

void start() {
  using namespace sys;
  if (hasfeature(CPU_YMM) && sys::hasfeature(CPU_AVX)) {
    con::out("rt: avx path selected");
    LOAD(rt::avx);
  } else if (hasfeature(CPU_SSE) && hasfeature(CPU_SSE2)) {
    con::out("rt: sse path selected");
    LOAD(rt::sse);
  } else {
    con::out("rt: warning slow path chosen for isosurface extraction");
    LOAD(rt);
  }
}
void finish() {world=NULL;}

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
static const vec3f lpos(35.f, 10.f, 11.f);
//static const vec3f lpos(0.f, 4.f, 0.f);
static atomic totalraynum;
struct task_raycast : public task {
  task_raycast(intersector *bvhisec, const camera &cam, int *pixels, vec2i dim, vec2i tile) :
    task("task_raycast", tile.x*tile.y, 1, 0, UNFAIR),
    bvhisec(bvhisec), cam(cam), pixels(pixels), dim(dim), tile(tile)
  {}
  INLINE u32 primarypoint(vec2i tileorg, array3f &pos, array3f &nor, arrayi &mask) {
    raypacket p;
    packethit hit;
    rtvisibilitypacket(cam, p, tileorg, dim);
    rtclearpackethit(hit);
    rtclosest(*bvhisec, p, hit);
    return rtprimarypoint(p, hit, pos, nor, mask);
  }
  virtual void run(u32 tileID) {
    const vec2i tilexy(tileID%tile.x, tileID/tile.x);
    const vec2i tileorg = int(TILESIZE) * tilexy;

#if NORMAL_ONLY
    // primary intersections
    raypacket p;
    packethit hit;
    rtvisibilitypacket(cam, p, tileorg, dim);
    rtclearpackethit(hit);
    rtclosest(*bvhisec, p, hit);
    rtwritenormal(hit, tileorg, dim, pixels);
    totalraynum += TILESIZE*TILESIZE;
#else
    // shadow rays toward the light source
    array3f pos, nor;
    arrayi mask;
    raypacket shadow;
    packetshadow occluded;
    const auto validnum = primarypoint(tileorg, pos, nor, mask);
    if (validnum == 0) {
      rtclear(tileorg, dim, pixels);
      totalraynum += TILESIZE*TILESIZE;
    } else {
      //const auto sec = game::lastmillis()/1000.f;
      const auto newpos = lpos;// + vec3f(10.f*sin(sec),0.f, 10.f*cos(sec));
      rtshadowpacket(pos, mask, newpos, shadow, occluded, TILESIZE*TILESIZE);
      rtoccluded(*bvhisec, shadow, occluded);
      rtwritendotl(shadow, nor, occluded, tileorg, dim, pixels);
      totalraynum += shadow.raynum+TILESIZE*TILESIZE;
    }
#endif
  }
  intersector *bvhisec;
  const camera &cam;
  int *pixels;
  vec2i dim;
  vec2i tile;
};

void raytrace(int *pixels, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect)
{
  const mat3x3f r = mat3x3f::rotate(-ypr.x,vec3f(0.f,1.f,0.f))*
                    mat3x3f::rotate(-ypr.y,vec3f(1.f,0.f,0.f))*
                    mat3x3f::rotate(-ypr.z,vec3f(0.f,0.f,1.f));
  const camera cam(pos, -r.vy, -r.vz, fovy, aspect);
  const vec2i dim(w,h), tile(dim/int(TILESIZE));
  totalraynum=0;
  ref<task> raycast = NEW(task_raycast, world, cam, pixels, dim, tile);
  raycast->scheduled();
  raycast->wait();
}

static int *pixels=NULL;
void raytrace(const char *bmp, const vec3f &pos, const vec3f &ypr,
              int w, int h, float fovy, float aspect)
{
  if (pixels==NULL) pixels = (int*)ALIGNEDMALLOC(w*h*sizeof(int), CACHE_LINE_ALIGNMENT);
  const auto start = sys::millis();
  raytrace(pixels, pos, ypr, w, h, fovy, aspect);
  const auto duration = float(sys::millis()-start);
  con::out("rt: %i ms, %f Mray/s", int(duration), 1000.f*(float(totalraynum)*1e-6f)/duration);
  sys::writebmp(pixels, w, h, bmp);
}
} /* namespace rt */
} /* namespace q */

