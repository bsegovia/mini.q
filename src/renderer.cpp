/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "csg.hpp"
#include "demo.hpp"
#include "game.hpp"
#include "iso_voxel.hpp"
#include "menu.hpp"
#include "md2.hpp"
#include "monster.hpp"
#include "network.hpp"
#include "rt.hpp"
#include "renderer.hpp"
#include "shaders.hpp"
#include "sky.hpp"
#include "text.hpp"
#include "weapon.hpp"
#include "base/console.hpp"
#include "base/script.hpp"

namespace q {
extern int fov;
namespace rr {

/*--------------------------------------------------------------------------
 - particle rendering
 -------------------------------------------------------------------------*/
static const int MAXPARTICLES = 10500;
// static const int NUMPARTCUTOFF = 20;
struct particle { vec3f o, d; int fade, type; int millis; particle *next; };
static particle particles[MAXPARTICLES], *parlist = NULL, *parempty = NULL;
static bool parinit = false;

VARP(maxparticles, 100, 2000, MAXPARTICLES-500);
VAR(demotracking, 0, 0, 1);
VARP(particlesize, 20, 100, 500);

static void newparticle(const vec3f &o, const vec3f &d, int fade, int type) {
  if (!parinit) {
    loopi(MAXPARTICLES) {
      particles[i].next = parempty;
      parempty = &particles[i];
    }
    parinit = true;
  }
  if (parempty) {
    particle *p = parempty;
    parempty = p->next;
    p->o = o;
    p->d = d;
    p->fade = fade;
    p->type = type;
    p->millis = int(game::lastmillis());
    p->next = parlist;
    parlist = p;
  }
}

static const struct parttype {vec3f rgb; int gr, tex; float sz;} parttypes[] = {
  {vec3f(0.7f, 0.6f, 0.3f), 2,  ogl::TEX_MARTIN_BASE,  0.06f}, // yellow: sparks
  {vec3f(0.5f, 0.5f, 0.5f), 20, ogl::TEX_MARTIN_SMOKE, 0.15f}, // grey:   small smoke
  {vec3f(0.2f, 0.2f, 1.0f), 20, ogl::TEX_MARTIN_BASE,  0.08f}, // blue:   edit mode entities
  {vec3f(1.0f, 0.1f, 0.1f), 1,  ogl::TEX_MARTIN_SMOKE, 0.06f}, // red:    blood spats
  {vec3f(1.0f, 0.8f, 0.8f), 20, ogl::TEX_MARTIN_BALL1, 1.2f }, // yellow: fireball1
  {vec3f(0.5f, 0.5f, 0.5f), 20, ogl::TEX_MARTIN_SMOKE, 0.6f }, // grey:   big smoke
  {vec3f(1.0f, 1.0f, 1.0f), 20, ogl::TEX_MARTIN_BALL2, 1.2f }, // blue:   fireball2
  {vec3f(1.0f, 1.0f, 1.0f), 20, ogl::TEX_MARTIN_BALL3, 1.2f }, // green:  fireball3
  {vec3f(1.0f, 0.1f, 0.1f), 0,  ogl::TEX_MARTIN_SMOKE, 0.2f }, // red:    demotrack
};
static const int parttypen = ARRAY_ELEM_NUM(parttypes);

typedef array<float,8> glparticle; // TODO use a more compressed format than that
static u32 particleibo = 0u, particlevbo = 0u;
static const int glindexn = 6*MAXPARTICLES, glvertexn = 4*MAXPARTICLES;
static glparticle *glparts = NULL;

static void initparticles(void) {
  // indices never change we set them once here
  const u16 twotriangles[] = {0,1,2,2,3,1};
  u16 *indices = NEWAE(u16, glindexn);
  ogl::genbuffers(1, &particleibo);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, particleibo);
  loopi(MAXPARTICLES) loopj(6) indices[6*i+j]=4*i+twotriangles[j];
  OGL(BufferData, GL_ELEMENT_ARRAY_BUFFER, glindexn*sizeof(u16), indices, GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);

  // vertices will be created at each drawing call
  ogl::genbuffers(1, &particlevbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, particlevbo);
  OGL(BufferData, GL_ARRAY_BUFFER, glvertexn*sizeof(glparticle), NULL, GL_DYNAMIC_DRAW);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  SAFE_DELA(indices);

  // Init the array dynamically since VS2012 crashes with a static declaration
  glparts = NEWAE(glparticle, glvertexn);
}

#if !defined(RELEASE)
static void cleanparticles(void) {
  if (particleibo) {
    ogl::deletebuffers(1, &particleibo);
    particleibo = 0;
  }
  if (particlevbo) {
    ogl::deletebuffers(1, &particlevbo);
    particlevbo = 0;
  }
  SAFE_DELA(glparts);
}
#endif
#if 0
static void render_particles(int time) {
  if (demo::playing() && demotracking) {
    const vec3f nom(0, 0, 0);
    newparticle(game::player1->o, nom, 100000000, 8);
  }

  // bucket sort the particles
  u32 partbucket[parttypen], partbucketsize[parttypen];
  loopi(parttypen) partbucketsize[i] = 0;
  for (particle *p, **pp = &parlist; (p = *pp);) {
    pp = &p->next;
    partbucketsize[p->type]++;
  }
  partbucket[0] = 0;
  loopi(parttypen-1) partbucket[i+1] = partbucket[i]+partbucketsize[i];

  // copy the particles to the vertex buffer
  int numrender = 0;
  const vec3f right(game::mvmat.vx.x,game::mvmat.vy.x,game::mvmat.vz.x);
  const vec3f up(game::mvmat.vx.y,game::mvmat.vy.y,game::mvmat.vz.y);
  for (particle *p, **pp = &parlist; (p = *pp);) {
    const u32 index = 4*partbucket[p->type]++;
    const auto *pt = &parttypes[p->type];
    const auto sz = pt->sz*particlesize/100.0f;
    glparts[index+0] = glparticle(pt->rgb, 0.f, 1.f, p->o-(right-up)*sz);
    glparts[index+1] = glparticle(pt->rgb, 1.f, 1.f, p->o+(right+up)*sz);
    glparts[index+2] = glparticle(pt->rgb, 0.f, 0.f, p->o-(right+up)*sz);
    glparts[index+3] = glparticle(pt->rgb, 1.f, 0.f, p->o-(up-right)*sz);
    if (numrender++>maxparticles || (p->fade -= time)<0) {
      *pp = p->next;
      p->next = parempty;
      parempty = p;
    } else {
      if (pt->gr)
        p->o.z -= float(((game::lastmillis()-p->millis)/3.0)*game::curtime()/(double(pt->gr)*10000));
      vec3f a = p->d;
      a *= float(time);
      a /= 20000.f;
      p->o += a;
      pp = &p->next;
    }
  }

  // render all of them now
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX|ogl::FIXED_COLOR);
  partbucket[0] = 0;
  loopi(parttypen-1) partbucket[i+1] = partbucket[i]+partbucketsize[i];
  OGL(DepthMask, GL_FALSE);
  ogl::enable(GL_BLEND);
  OGL(BlendFunc, GL_SRC_ALPHA, GL_SRC_ALPHA);
  ogl::setattribarray()(ogl::ATTRIB_POS0, ogl::ATTRIB_COL, ogl::ATTRIB_TEX0);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, particlevbo);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, particleibo);
  OGL(BufferSubData, GL_ARRAY_BUFFER, 0, numrender*sizeof(glparticle[4]), glparts);
  OGL(VertexAttribPointer, ogl::ATTRIB_COL, 3, GL_FLOAT, 0, sizeof(glparticle), (const void*)0);
  OGL(VertexAttribPointer, ogl::ATTRIB_TEX0, 2, GL_FLOAT, 0, sizeof(glparticle), (const void*)sizeof(float[3]));
  OGL(VertexAttribPointer, ogl::ATTRIB_POS0, 3, GL_FLOAT, 0, sizeof(glparticle), (const void*)sizeof(float[5]));
  loopi(parttypen) {
    if (partbucketsize[i] == 0) continue;
    const auto pt = &parttypes[i];
    const int n = partbucketsize[i]*6;
    ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(pt->tex));
    const auto offset = (const void *) (partbucket[i] * sizeof(u16[6]));
    ogl::fixedflush();
    ogl::drawelements(GL_TRIANGLES, n, GL_UNSIGNED_SHORT, offset);
  }

  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);
  ogl::disable(GL_BLEND);
  OGL(DepthMask, GL_TRUE);
}
#endif

void particle_splash(int type, int num, int fade, const vec3f &p) {
  loopi(num) {
    const int radius = type==5 ? 50 : 150;
    int x, y, z;
    do {
      x = rnd(radius*2)-radius;
      y = rnd(radius*2)-radius;
      z = rnd(radius*2)-radius;
    } while (x*x+y*y+z*z>radius*radius);
    const vec3f d = vec3f(float(x), float(y), float(z));
    newparticle(p, d, rnd(fade*3), type);
  }
}

void particle_trail(int type, int fade, const vec3f &s, const vec3f &e) {
  const vec3f v = e-s;
  const float d = length(v);
  const vec3f nv = v/(d*2.f+0.1f);
  vec3f p = s;
  loopi(int(d)*2) {
    p += nv;
    const vec3f d(float(rnd(12)-5), float(rnd(11)-5), float(rnd(11)-5));
    newparticle(p, d, rnd(fade)+fade, type);
  }
}

/*--------------------------------------------------------------------------
 - HUD transformation and dimensions
 -------------------------------------------------------------------------*/
float VIRTH = 1.f;
vec2f scrdim() { return vec2f(float(sys::scrw), float(sys::scrh)); }
static void pushscreentransform() {
  const auto scr = scrdim();
  ogl::pushmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::pushmode(ogl::PROJECTION);
  ogl::setortho(0.f, scr.x, 0.f, scr.y, -1.f, 1.f);
}
static void popscreentransform() {
  ogl::popmode(ogl::PROJECTION);
  ogl::popmode(ogl::MODELVIEW);
}

/*--------------------------------------------------------------------------
 - simple primitives
 -------------------------------------------------------------------------*/
static const float ICON_SIZE = 80.f, ICON_TEX_SIZE = 192.f;
void drawicon(float tx, float ty, float x, float y) {
  const auto o = 1.f/3.f;
  const auto s = ICON_SIZE;
  tx /= ICON_TEX_SIZE;
  ty /= ICON_TEX_SIZE;
  const float verts[] = {
    tx,   ty+o, x,   y,
    tx+o, ty+o, x+s, y,
    tx,   ty,   x,   y+s,
    tx+o, ty,   x+s, y+s
  };
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_ITEM), 0);
  ogl::immdraw("St2p2", 4, verts);
}

void blendbox(float x1, float y1, float x2, float y2, bool border) {
  ogl::enablev(GL_BLEND);
  OGL(DepthMask, GL_FALSE);
  OGL(BlendFunc, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
  ogl::setattribarray()(ogl::ATTRIB_POS0);
  ogl::bindfixedshader(ogl::FIXED_COLOR);
  if (border)
    OGL(VertexAttrib3f, ogl::ATTRIB_COL, .5f, .3f, .4f);
  else
    OGL(VertexAttrib3f, ogl::ATTRIB_COL, 1.f, 1.f, 1.f);
  const float verts0[] = {x1, y1, x2, y1, x1, y2, x2, y2};
  ogl::immdraw("Sp2", 4, verts0);

  ogl::disablev(GL_BLEND);
  OGL(VertexAttrib3f, ogl::ATTRIB_COL, .2f, .7f, .4f);
  const float verts1[] = {x1, y1, x2, y1, x2, y2, x1, y2};
  ogl::immdraw("Lp2", 4, verts1);

  OGL(DepthMask, GL_TRUE);
  ogl::enablev(GL_BLEND);
  OGL(BlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/*--------------------------------------------------------------------------
 - handle the HUD (console, scores...)
 -------------------------------------------------------------------------*/
VAR(showstats, 0, 0, 1);
static void drawhud(int w, int h, int curfps) {
  const auto scr = scrdim();
  const auto fontdim = text::fontdim();
  ogl::enablev(GL_BLEND);
  ogl::disable(GL_DEPTH_TEST);
  OGL(BlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  pushscreentransform();
  text::displaywidth(text::fontdim().x);
  OGL(VertexAttrib4f,ogl::ATTRIB_COL,1.f,1.f,1.f,1.f);
  con::render();
  if (showstats) {
    vec2f textpos(scr.x-400.f, scr.y-50.f);
    const auto o = game::player1->o;
    const auto ypr = game::player1->ypr;
    text::drawf("x: %f y: %f z: %f", textpos, o.x, o.y, o.z);
    textpos.y += fontdim.y;
    text::drawf("yaw: %f pitch: %f roll: %f", textpos, ypr.x, ypr.y, ypr.z);
    textpos.y += fontdim.y;
    text::drawf("%i f/s", textpos, curfps);
    ogl::printtimers(text::fontdim().x, con::height());
  }
  menu::render();

  if (game::player1->state==CS_ALIVE) {
    const auto lifepos = vec2f(float(VIRTW) * 0.05f,ICON_SIZE*0.2f);
    const auto armorpos = vec2f(float(VIRTW) * 0.30f, ICON_SIZE*0.2f);
    const auto ammopos = vec2f(float(VIRTW) * 0.55f, ICON_SIZE*0.2f);
    const auto textpos = vec2f(1.2f*ICON_SIZE, -ICON_SIZE*0.2f);
    text::displaywidth(ICON_SIZE/text::fontratio());
    const auto pl = game::player1;
    ogl::pushmode(ogl::PROJECTION);
    ogl::setortho(0.f, float(VIRTW), 0.f, float(VIRTH), -1.f, 1.f);
    OGL(VertexAttrib4f,ogl::ATTRIB_COL,1.f,1.f,1.f,1.f);
    text::drawf("%d", lifepos+textpos, pl->health);
    if (pl->armour) text::drawf("%d", armorpos+textpos, pl->armour);
    text::drawf("%d", ammopos+textpos, pl->ammo[pl->gunselect]);
    ogl::disablev(GL_BLEND);
    drawicon(128, 128, lifepos.x, lifepos.y);
    if (pl->armour)
      drawicon(float(pl->armourtype)*64.f, 0, armorpos.x, armorpos.y);
    auto g = float(pl->gunselect);
    auto r = 64.f;
    if (g>2) {
      g -= 3.f;
      r = 128.f;
    }
    drawicon(g*64.f, r, ammopos.x, ammopos.y);
    ogl::popmatrix();
    text::resetdefaultwidth();
  }

  ogl::disablev(GL_BLEND);
  popscreentransform();
  ogl::enable(GL_DEPTH_TEST);
}

/*--------------------------------------------------------------------------
 - handle the HUD gun
 -------------------------------------------------------------------------*/
#if 0
static const char *hudgunnames[] = {
  "hudguns/fist",
  "hudguns/shotg",
  "hudguns/chaing",
  "hudguns/rocket",
  "hudguns/rifle"
};
VARP(showhudgun, 0, 1, 1);

static void drawhudmodel(int start, int end, float speed, int base) {
  const auto pl = game::player1;
  const auto norxfm = mat3x3f::rotate(-pl->ypr.x, vec3f(0.f,1.f,0.f))
                    * mat3x3f::rotate(-pl->ypr.y, vec3f(1.f,0.f,0.f))
                    * mat3x3f::rotate(-pl->ypr.z, vec3f(0.f,0.f,1.f));
  md2::render(hudgunnames[pl->gunselect], start, end,
              mat4x4f(one), norxfm, false, 1.0f, speed, 0, float(base));
}

static void drawhudgun(float fovy, float aspect, float farplane) {
  if (!showhudgun) return;
  const int rtime = game::reloadtime(game::player1->gunselect);
  if (game::player1->lastaction &&
      game::player1->lastattackgun==game::player1->gunselect &&
      game::lastmillis()-game::player1->lastaction<rtime)
    drawhudmodel(7, 18, rtime/18.0f, game::player1->lastaction);
  else
    drawhudmodel(6, 1, 100.f, 0);
}
#endif

static void texbufrules(ogl::shaderrules &vert, ogl::shaderrules &frag, u32) {
  ogl::fixedrules(vert,frag,0);
}
#define RULES texbufrules
#define SHADERNAME texbuf
#define VERTEX_PROGRAM "data/shaders/fixed_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/texbuf_fp.decl"
#include "shaderdecl.hxx"
#undef RULES

static u32 rttex, rtpbo;
static void initrt() {
  const auto dim = scrdim();
  auto w = int(dim.x), h = int(dim.y);
  // no texture buffer here
  if (!ogl::hasTB) {
    ogl::genbuffers(1, &rtpbo);
    ogl::gentextures(1, &rttex);
    ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, rtpbo);
    OGL(BufferData,GL_PIXEL_UNPACK_BUFFER, w * h * sizeof(u32), NULL, GL_STATIC_DRAW);
    ogl::bindtexture(GL_TEXTURE_2D, rttex, 0);
    OGL(TexImage2D,GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    OGL(PixelStorei,GL_UNPACK_ALIGNMENT, 1);
    OGL(TexParameteri,GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    OGL(TexParameteri,GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    OGL(TexParameteri,GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    OGL(TexParameteri,GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    ogl::bindtexture(GL_TEXTURE_2D, 0, 0);
    ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, 0);
  }
  // faster path. we use a texture buffer for the raytracing output
  else {
    ogl::gentextures(1, &rttex);
    ogl::genbuffers(1, &rtpbo);
    ogl::bindbuffer(ogl::TEXTURE_BUFFER, rtpbo);
    OGL(BufferData, GL_TEXTURE_BUFFER, w * h * sizeof(u32), NULL, GL_STATIC_DRAW);
    ogl::bindbuffer(ogl::TEXTURE_BUFFER, 0);
  }
}

#if !defined(RELEASE)
static void cleanrt() {
  if (rttex) ogl::deletetextures(1, &rttex);
  if (rtpbo) ogl::deletebuffers(1, &rtpbo);
}
#endif

static void *pbomap(u32 pbo) {
  const auto dim = scrdim();
  void *ptr;
  ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, pbo);
  OGL(BufferData,GL_PIXEL_UNPACK_BUFFER, int(dim.x) * int(dim.y) * sizeof(u32), NULL, GL_STATIC_DRAW);
  OGLR(ptr, MapBuffer, GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  assert(ptr != NULL);
  ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, 0);
  return ptr;
}

static void pbounmap(u32 pbo, u32 tex) {
  const auto dim = scrdim();
  ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, pbo);
  OGL(UnmapBuffer, GL_PIXEL_UNPACK_BUFFER);
  ogl::bindtexture(GL_TEXTURE_2D, tex, 0);
  OGL(TexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, int(dim.x), int(dim.y), GL_BGRA, GL_UNSIGNED_BYTE, 0);
  ogl::bindbuffer(ogl::PIXEL_UNPACK_BUFFER, 0);
  ogl::bindtexture(GL_TEXTURE_2D, 0, 0);
}

static void *texbufmap(u32 texbuf) {
  const auto dim = scrdim();
  void *ptr;
  ogl::bindbuffer(ogl::TEXTURE_BUFFER, texbuf);
  OGL(BufferData,GL_TEXTURE_BUFFER, int(dim.x) * int(dim.y) * sizeof(u32), NULL, GL_DYNAMIC_DRAW);
  OGLR(ptr, MapBuffer, GL_TEXTURE_BUFFER, GL_WRITE_ONLY);
  assert(ptr!=NULL);
  ogl::bindbuffer(ogl::TEXTURE_BUFFER, 0);
  return ptr;
}

static void texbufunmap(u32 texbuf) {
  ogl::bindbuffer(ogl::TEXTURE_BUFFER, texbuf);
  OGL(UnmapBuffer, GL_TEXTURE_BUFFER);
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
static bool initialized_m = false;

void start() {
  initparticles();
  initrt();
  texbuf::s.fixedfunction = true;
}

#if !defined(RELEASE)
void finish() {
  cleanrt();
  cleanparticles();
}
#endif

static void voxel(const vec3f &org, u32 cellnum, float cellsize, const csg::node &root) {
  iso::voxel::octree o(cellnum);
  ref<task> voxel_task = iso::voxel::create_task(o, root, org, cellnum, cellsize);
  voxel_task->scheduled();
  voxel_task->wait();
}

static void voxelize(float d) {
  const auto node = csg::makescene();
  assert(node != NULL);
  const auto start = sys::millis();
  voxel(vec3f(zero), 2*8192, d, *node);
  const auto duration = sys::millis() - start;
  con::out("voxel: elapsed %f ms ", float(duration));
}
CMD(voxelize);

static const float CELLSIZE = 0.1f;
static void makescene() {
  if (initialized_m) return;
  voxelize(0.05f);
  initialized_m = true;
}

struct screenquad {
  static INLINE screenquad get() {
    const auto w = float(sys::scrw), h = float(sys::scrh);
    return screenquad({{w,h,w,0.f,0.f,h,0.f,0.f}});
  };
  static INLINE screenquad getnormalized() {
    return screenquad({{1.f,1.f,1.f,-1.f,-1.f,1.f,-1.f,-1.f}});
  };
  float v[16];
};

VAR(linemode, 0, 0, 1);

static void ogl2raytrace(int w, int h, float fov, float aspect) {
  const auto pos = game::player1->o;
  const auto ypr = game::player1->ypr;
  const auto starttotal = sys::millis();
  const auto pixels = (int*) pbomap(rtpbo);
  const auto start = sys::millis();
  rt::raytrace(pixels,pos,ypr,w,h,fov,aspect);
  const auto end = sys::millis();
  pbounmap(rtpbo, rttex);
  const auto endtotal = sys::millis();
  printf("\rrt %f total %f              ", float(end-start), float(endtotal-starttotal));
  ogl::bindfixedshader(ogl::FIXED_DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, rttex, 0);
  pushscreentransform();
  const float coords[]={
    float(w),float(h),0.f,1.f,
    float(w),0,0.f,0.f,
    0,float(h),1.f,1.f,
    0,0,1.f,0.f
  };
  ogl::immdraw("Sp2t2", 4, coords);
  popscreentransform();
}

static void ogl3raytrace(int w, int h, float fov, float aspect) {
  const auto pos = game::player1->o;
  const auto ypr = game::player1->ypr;
  const auto starttotal = sys::millis();
  const auto pixels = (int*) texbufmap(rtpbo);
  const auto start = sys::millis();
  rt::raytrace(pixels,pos,ypr,w,h,fov,aspect);
  const auto end = sys::millis();
  texbufunmap(rtpbo);
  const auto endtotal = sys::millis();
  printf("\rrt %f total %f              ", float(end-start), float(endtotal-starttotal));
  ogl::bindshader(texbuf::s);
  ogl::bindtexture(GL_TEXTURE_BUFFER, rttex, 0);
  OGL(TexBuffer, GL_TEXTURE_BUFFER, GL_RGBA8, rtpbo);
  OGL(Uniform1i, texbuf::s.u_width, w);
  pushscreentransform();
  ogl::immdraw("Sp2", 4, screenquad::get().v);
  popscreentransform();
}

void frame(int w, int h, int curfps) {
  const auto aspect = float(w) / float(h);
  const auto fovy = float(fov) / aspect;
  const auto rttimer = ogl::begintimer("rt", true);
  makescene();
  ogl::disable(GL_CULL_FACE);
  OGL(DepthMask, GL_FALSE);
  if (ogl::hasTB)
    ogl3raytrace(w,h,fovy,aspect);
  else
    ogl2raytrace(w,h,fovy,aspect);
  ogl::enable(GL_CULL_FACE);
  OGL(DepthMask, GL_TRUE);
  ogl::endtimer(rttimer);

  const auto hudtimer = ogl::begintimer("hud", true);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
  ogl::endtimer(hudtimer);
}
} /* namespace rr */
} /* namespace q */

