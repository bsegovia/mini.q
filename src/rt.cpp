/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - rt.cpp -> implements ray tracing kernels
 -------------------------------------------------------------------------*/
#include "bvh.hpp"
#include "bvhsse.hpp"
#include "bvhavx.hpp"
#include "rt.hpp"
#include "base/math.hpp"
#include "base/console.hpp"
#include "base/task.hpp"

namespace q {
namespace rt {
static bvh::intersector *world = NULL;

// create a triangle soup and make a mesh out of it
void buildbvh(vec3f *v, u32 *idx, u32 idxnum) {
  const auto start = sys::millis();
  const auto trinum = idxnum/3;
  const auto prim = NEWAE(bvh::primitive, trinum);
  loopi(trinum) {
    loopj(3) prim[i].v[j] = v[idx[3*i+j]];
    prim[i].type = bvh::primitive::TRI;
  }
  world = bvh::create(prim, trinum);
  const auto ms = sys::millis() - start;
  con::out("bvh: elapsed %f ms", float(ms));
}

enum { TILESIZE = 16 };
static const vec3f lpos(0.f, -4.f, 0.f);
static atomic totalraynum;
struct raycasttask : public task {
  raycasttask(bvh::intersector *bvhisec, const camera &cam, int *pixels, vec2i dim, vec2i tile) :
    task("raycasttask", tile.x*tile.y, 1, 0, UNFAIR),
    bvhisec(bvhisec), cam(cam), pixels(pixels), dim(dim), tile(tile)
  {}
  virtual void run(u32 tileID) {
    const vec2i tilexy(tileID%tile.x, tileID/tile.x);
    const vec2i screen = int(TILESIZE) * tilexy;
    bvh::raypacket p;
    vec3f mindir(FLT_MAX), maxdir(-FLT_MAX);
    for (u32 y = 0; y < u32(TILESIZE); ++y)
    for (u32 x = 0; x < u32(TILESIZE); ++x) {
      const ray ray = cam.generate(dim.x, dim.y, screen.x+x, screen.y+y);
      const int idx = x+y*TILESIZE;
      p.setdir(ray.dir, idx);
      p.setorg(cam.org, idx);
      mindir = min(mindir, ray.dir);
      maxdir = max(maxdir, ray.dir);
    }
    p.raynum = TILESIZE*TILESIZE;
    p.flags = bvh::raypacket::COMMONORG;
    if (all(gt(mindir*maxdir,vec3f(zero)))) {
      p.iadir = makeinterval(mindir, maxdir);
      p.iardir = rcp(p.iadir);
      p.iaorg = makeinterval(cam.org, cam.org);
      p.iamaxlen = FLT_MAX;
      p.iaminlen = 0.f;
      p.flags |= bvh::raypacket::INTERVALARITH;
    }

    bvh::packethit hit;
    loopi(bvh::MAXRAYNUM) {
      hit.id[i] = ~0x0u;
      hit.t[i] = FLT_MAX;
    }
    bvh::avx::closest(*bvhisec, p, hit);

#define NORMAL_ONLY 1
#if !NORMAL_ONLY
    // exclude points that interesect nothing
    int mapping[TILESIZE*TILESIZE], curr = 0;
    bvh::raypacket shadow;
    bvh::packetshadow occluded;
    float maxlen = -FLT_MAX;
    mindir = vec3f(FLT_MAX), maxdir = vec3f(-FLT_MAX);
    loopi(int(p.raynum)) {
      if (hit.ishit(i)) {
        mapping[i] = curr;
        const auto n = normalize(hit.getnormal(i));
        hit.n[0][i] = n.x;
        hit.n[1][i] = n.y;
        hit.n[2][i] = n.z;
        const auto dst = p.org(i) + hit.t[i] * p.dir(i) + n * 1e-2f;
        const auto dir = dst-lpos;
        const auto len = length(dir);
        maxlen = max(maxlen, len);
        shadow.setorg(lpos, curr);
        shadow.setdir(dir/len, curr);
        mindir = min(mindir, shadow.dir(curr));
        maxdir = max(maxdir, shadow.dir(curr));
        occluded.t[curr] = len;
        occluded.occluded[curr] = 0;
        ++curr;
      } else
        mapping[i] = -1;
    }
    shadow.raynum = curr;
    shadow.flags = bvh::raypacket::COMMONORG;
    if (all(gt(mindir*maxdir,vec3f(zero)))) {
      shadow.iadir = makeinterval(mindir, maxdir);
      shadow.iardir = rcp(shadow.iadir);
      shadow.iaorg = makeinterval(lpos,lpos);
      shadow.flags |= bvh::raypacket::INTERVALARITH;
      shadow.iamaxlen = maxlen;
      shadow.iaminlen = 0.f;
    }
    bvh::occluded(*bvhisec, shadow, occluded);

    for (u32 y = 0; y < u32(TILESIZE); ++y)
    for (u32 x = 0; x < u32(TILESIZE); ++x) {
      const int offset = (screen.x+x)+dim.x*(screen.y+y);
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
      const int offset = (screen.x+x)+dim.x*(screen.y+y);
      const int idx = x+y*TILESIZE;
      if (hit.ishit(idx)) {
        const auto n = vec3i(abs(normalize(hit.getnormal(idx))*255.f));
        pixels[offset] = n.x|(n.y<<8)|(n.z<<16)|(0xff<<24);
      } else
        pixels[offset] = 0;
    }
    totalraynum += TILESIZE*TILESIZE;
#endif
  }
  bvh::intersector *bvhisec;
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

