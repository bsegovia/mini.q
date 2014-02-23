/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "iso.hpp"
#include "sky.hpp"
#include "csg.hpp"
#include "shaders.hpp"

namespace q {
namespace rr {

/*--------------------------------------------------------------------------
 - particle rendering
 -------------------------------------------------------------------------*/
static const int MAXPARTICLES = 10500;
static const int NUMPARTCUTOFF = 20;
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
    p->millis = game::lastmillis();
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
        p->o.z -= ((game::lastmillis()-p->millis)/3.0f)*game::curtime()/(pt->gr*10000);
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
              mat4x4f(one), norxfm, false, 1.0f, speed, 0, base);
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

/*--------------------------------------------------------------------------
 - deferred shading stuff
 -------------------------------------------------------------------------*/
#define SPLITNUM 4

static void deferredrules(ogl::shaderrules &vert, ogl::shaderrules &frag, u32 rule) {
  string str;
  sprintf_s(str)("#define LIGHTNUM %d\n", rule+1);
  frag.insert(0, NEWSTRING(str));
  frag.add(NEWSTRING("vec3 shade(vec3 pos, vec3 nor) {\n"));
  frag.add(NEWSTRING("  vec3 outcol = diffuse(pos,nor,u_lightpos[0],u_lightpow[0]);\n"));
  loopi(int(rule)) {
    sprintf_s(str)("  outcol += diffuse(pos,nor,u_lightpos[%d],u_lightpow[%d]);\n",i+1,i+1);
    frag.add(NEWSTRING(str));
  }
  frag.add(NEWSTRING("  return outcol;\n}\n"));
}

#define SHADERVARIANT 16
#define RULES deferredrules
#define SHADERNAME deferred
#define VERTEX_PROGRAM "data/shaders/deferred_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/deferred_fp.decl"
#include "shaderdecl.hxx"
#undef RULES

static void rules(ogl::shaderrules &vert, ogl::shaderrules &frag, u32 rule) {
  sprintf_sd(str)("#define SPLITNUM %f\n", float(SPLITNUM));
  vert.add(NEWSTRING(str));
  frag.add(NEWSTRING(str));
}

#define RULES rules
#define SHADERNAME split_deferred
#define VERTEX_PROGRAM "data/shaders/split_deferred_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/split_deferred_fp.decl"
#include "shaderdecl.hxx"

#define SHADERNAME forward
#define VERTEX_PROGRAM "data/shaders/forward_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/forward_fp.decl"
#include "shaderdecl.hxx"

#define SHADERNAME debugunsplit
#define VERTEX_PROGRAM "data/shaders/debugunsplit_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/debugunsplit_fp.decl"
#include "shaderdecl.hxx"

#define SHADERNAME simple_material
#define VERTEX_PROGRAM "data/shaders/simple_material_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/simple_material_fp.decl"
#include "shaderdecl.hxx"

#define SHADERNAME fxaa
#define VERTEX_PROGRAM "data/shaders/fxaa_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/fxaa_fp.decl"
#include "shaderdecl.hxx"
#undef RULES

static void shadertoyrules(ogl::shaderrules &vert, ogl::shaderrules &frag, u32) {
  if (ogl::loadfromfile()) {
    auto s = sys::loadfile("data/shaders/hell.glsl");
    assert(s);
    frag.add(s);
  } else
    frag.add(NEWSTRING(shaders::hell));
}

#define RULES shadertoyrules
#define SHADERNAME hell
#define VERTEX_PROGRAM "data/shaders/shadertoy_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/shadertoy_fp.decl"
#include "shaderdecl.hxx"
#undef RULES

static u32 gdepthtex, gnortex, gdiffusetex, finaltex;
static u32 gbuffer, shadedbuffer;

static void initdeferred() {
  // all textures
  gnortex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  gdiffusetex = ogl::maketex("TB I3 D3 Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);
  finaltex = ogl::maketex("TB I3 D3 B2 Wse Wte ml Ml", NULL, sys::scrw, sys::scrh);
  gdepthtex = ogl::maketex("Tf Id Dd Br Wse Wte mn Mn", NULL, sys::scrw, sys::scrh);

  // all frame buffer objects
  ogl::genframebuffers(1, &gbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, gdiffusetex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, gnortex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, gdepthtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init gbuffer framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
  ogl::genframebuffers(1, &shadedbuffer);
  OGL(BindFramebuffer, GL_FRAMEBUFFER, shadedbuffer);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finaltex, 0);
  OGL(FramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_RECTANGLE, gdepthtex, 0);
  if (GL_FRAMEBUFFER_COMPLETE != ogl::CheckFramebufferStatus(GL_FRAMEBUFFER))
    sys::fatal("renderer: unable to init debug unsplit framebuffer");
  OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
}

#if !defined(RELEASE)
static void cleandeferred() {
  ogl::deletetextures(1, &gdiffusetex);
  ogl::deletetextures(1, &gnortex);
  ogl::deletetextures(1, &gdepthtex);
  ogl::deletetextures(1, &finaltex);
  ogl::deleteframebuffers(1, &gbuffer);
  ogl::deleteframebuffers(1, &shadedbuffer);
}
#endif

/*--------------------------------------------------------------------------
 - sky parameters
 -------------------------------------------------------------------------*/
static const auto latitude = 42.f;
static const auto longitude = 1.f;
static const auto utc_hour = 7.8f;
static vec3f getsundir() {
  const auto jglobaltime = 1.f+sys::millis()*1e-3f*.75f;
  const auto season = mod(jglobaltime, 4.f)/4.f;
  const auto day = floor(season*365.f)+100.f;
  const auto jd2000 = sky::julianday2000((day*24.f+utc_hour)*3600.f);
  return sky::sunvector(jd2000, latitude, longitude);
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
static u32 scenenorbo = 0u, sceneposbo = 0u, sceneibo = 0u;
static u32 indexnum = 0u;
static bool initialized_m = false;

void start() {
  initdeferred();
  initparticles();
}
#if !defined(RELEASE)
void finish() {
  if (initialized_m) {
    ogl::deletebuffers(1, &sceneposbo);
    ogl::deletebuffers(1, &scenenorbo);
    ogl::deletebuffers(1, &sceneibo);
  }
  cleandeferred();
  cleanparticles();
}
#endif

static const float CELLSIZE = 0.2f;
static void makescene() {
  if (initialized_m) return;
  const float start = sys::millis();
  const auto node = csg::makescene();
  auto m = iso::dc_mesh_mt(vec3f(zero), 2048, CELLSIZE, *node);
  ogl::genbuffers(1, &sceneposbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, sceneposbo);
  OGL(BufferData, GL_ARRAY_BUFFER, m.m_vertnum*sizeof(vec3f), &m.m_pos[0].x, GL_STATIC_DRAW);
  ogl::genbuffers(1, &scenenorbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, scenenorbo);
  OGL(BufferData, GL_ARRAY_BUFFER, m.m_vertnum*sizeof(vec3f), &m.m_nor[0].x, GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::genbuffers(1, &sceneibo);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, sceneibo);
  OGL(BufferData, GL_ELEMENT_ARRAY_BUFFER, m.m_indexnum*sizeof(u32), &m.m_index[0], GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);
  indexnum = m.m_indexnum;

  const auto duration = sys::millis() - start;
  con::out("csg: elapsed %f ms ", duration);
  con::out("csg: tris %i verts %i", m.m_indexnum/3, m.m_vertnum);
  initialized_m = true;
  destroyscene(node);
}

struct screenquad {
  static INLINE screenquad get() {
    const auto w = float(sys::scrw), h = float(sys::scrh);
    return screenquad({{w,h,w,0.f,0.f,h,0.f,0.f}});
  };
  static INLINE screenquad getnormalized() {
    return screenquad({{1.f,1.f,1.f,-1.f,-1.f,1.f,-1.f,-1.f}});
  };
  static INLINE screenquad getsubbuffer(vec2i xy) {
    const auto w = float(sys::scrw/SPLITNUM);
    const auto h = float(sys::scrh/SPLITNUM);
    const auto sw = w*xy.x, sh = h*xy.y;
    const auto ew = sw+w, eh = sh+h;
    return screenquad({{ew,eh,ew,sh,sw,eh,sw,sh}});
  }
  float v[16];
};

// XXX just to have something to display
#define LIGHTNUM 16
VAR(lightscale, 0, 2000, 10000);
static const vec3f lightpos[LIGHTNUM] = {
  vec3f(8.f,20.f,8.f), vec3f(10.f,20.f,8.f),
  vec3f(12.f,20.f,8.f), vec3f(14.f,20.f,8.f),
  vec3f(6.f,20.f,8.f), vec3f(16.f,20.f,4.f),
  vec3f(16.f,21.f,7.f), vec3f(16.f,22.f,9.f),
  vec3f(8.f,20.f,9.f), vec3f(10.f,20.f,10.f),
  vec3f(12.f,20.f,12.f), vec3f(14.f,20.f,13.f),
  vec3f(6.f,20.f,14.f), vec3f(16.f,20.f,15.f),
  vec3f(15.f,21.f,7.f), vec3f(16.f,22.f,10.f)
};

static const vec3f lightpow[LIGHTNUM] = {
  vec3f(1.f,0.f,0.f), vec3f(0.f,1.f,0.f),
  vec3f(1.f,0.f,1.f), vec3f(1.f,1.f,1.f),
  vec3f(1.f,0.f,1.f), vec3f(0.f,1.f,1.f),
  vec3f(1.f,1.f,0.f), vec3f(0.f,1.f,1.f),
  vec3f(0.f,1.f,1.f), vec3f(1.f,0.f,0.f),
  vec3f(0.f,1.f,0.f), vec3f(1.f,0.f,1.f),
  vec3f(1.f,1.f,1.f), vec3f(1.f,0.f,1.f),
  vec3f(0.f,1.f,1.f), vec3f(1.f,1.f,0.f),
};

VAR(linemode, 0, 0, 1);

struct context {
  context(float w, float h, float fovy, float aspect, float farplane)
    : fovy(fovy), aspect(aspect), farplane(farplane)
  {}

  INLINE void begin() {
    makescene();
    OGL(Clear, GL_DEPTH_BUFFER_BIT);
  }
  INLINE void end() {}

  void dogbuffer() {
    const auto gbuffertimer = ogl::begintimer("gbuffer", true);
    const GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    OGL(BindFramebuffer, GL_FRAMEBUFFER, gbuffer);
    OGL(DrawBuffers, 2, buffers);
    OGL(Clear, GL_DEPTH_BUFFER_BIT);
    if (indexnum != 0) {
      if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_LINE);
      ogl::bindshader(simple_material::s);
      OGL(UniformMatrix4fv, simple_material::s.u_mvp, 1, GL_FALSE, &game::mvpmat.vx.x);
      ogl::bindbuffer(ogl::ARRAY_BUFFER, sceneposbo);
      OGL(VertexAttribPointer, ogl::ATTRIB_POS0, 3, GL_FLOAT, 0, sizeof(vec3f), NULL);
      ogl::bindbuffer(ogl::ARRAY_BUFFER, scenenorbo);
      OGL(VertexAttribPointer, ogl::ATTRIB_COL, 3, GL_FLOAT, 0, sizeof(vec3f), NULL);
      ogl::setattribarray()(ogl::ATTRIB_POS0, ogl::ATTRIB_COL);
      ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, sceneibo);
      ogl::drawelements(GL_TRIANGLES, indexnum, GL_UNSIGNED_INT, NULL);
      ogl::bindbuffer(ogl::ELEMENT_ARRAY_BUFFER, 0);
      ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
      if (linemode) OGL(PolygonMode, GL_FRONT_AND_BACK, GL_FILL);
    }
    game::renderclients();
    game::rendermonsters();
    drawhudgun(fovy, aspect, farplane);
    OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
    ogl::endtimer(gbuffertimer);
  }

  void dodeferred() {
    const auto deferredtimer = ogl::begintimer("deferred", true);
    OGL(BindFramebuffer, GL_FRAMEBUFFER, shadedbuffer);
    ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
    auto &s = deferred::s[LIGHTNUM-1];
    ogl::bindshader(s);
    ogl::bindtexture(GL_TEXTURE_RECTANGLE, gnortex, 0);
    ogl::bindtexture(GL_TEXTURE_RECTANGLE, gdiffusetex, 1);
    ogl::bindtexture(GL_TEXTURE_RECTANGLE, gdepthtex, 2);
    OGL(UniformMatrix4fv, s.u_invmvp, 1, GL_FALSE, &game::invmvpmat.vx.x);
    OGL(UniformMatrix4fv, s.u_dirinvmvp, 1, GL_FALSE, &game::dirinvmvpmat.vx.x);

    const auto sundir = getsundir();
    vec3f lpow[LIGHTNUM];
    loopi(LIGHTNUM) lpow[i] = float(lightscale) * lightpow[i];
    OGL(Uniform3fv, s.u_sundir, 1, &sundir.x);
    OGL(Uniform3fv, s.u_lightpos, LIGHTNUM, &lightpos[0].x);
    OGL(Uniform3fv, s.u_lightpow, LIGHTNUM, &lpow[0].x);
    ogl::immdraw("Sp2", 4, screenquad::getnormalized().v);
    ogl::enable(GL_DEPTH_TEST);

    render_particles(game::curtime());
    OGL(BindFramebuffer, GL_FRAMEBUFFER, 0);
    ogl::enable(GL_CULL_FACE);
    ogl::endtimer(deferredtimer);
  }

  void dofxaa() {
    const auto fxaatimer = ogl::begintimer("fxaa", true);
    ogl::bindshader(fxaa::s);
    ogl::disable(GL_CULL_FACE);
    OGL(DepthMask, GL_FALSE);
    const vec2f rcptexsize(1.f/float(sys::scrw), 1.f/float(sys::scrh));
    OGL(Uniform2fv, fxaa::s.u_rcptexsize, 1, &rcptexsize.x);
    ogl::bindtexture(GL_TEXTURE_2D, finaltex, 0);
    ogl::immdraw("Sp2", 4, screenquad::getnormalized().v);
    OGL(DepthMask, GL_TRUE);
    ogl::enable(GL_CULL_FACE);
    ogl::endtimer(fxaatimer);
  }

  float fovy, aspect, farplane;
};

VAR(shadertoy, 0, 0, 1);

static void doshadertoy(float fovy, float aspect, float farplane) {
  const auto shadertoytimer = ogl::begintimer("shadertoy", true);
  const auto w = float(sys::scrw), h = float(sys::scrh);
  ogl::bindshader(hell::s);
  ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
  OGL(DepthMask, GL_FALSE);
  const vec2f iResolution(w, h);
  const float sec = 1e-3f * sys::millis();
  OGL(Uniform3fv, hell::s.iResolution, 1, &iResolution.x);
  OGL(Uniform1f, hell::s.iGlobalTime, sec);
  ogl::immdraw("Sp2", 4, screenquad::getnormalized().v);
  OGL(DepthMask, GL_TRUE);
  ogl::enablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::endtimer(shadertoytimer);
}

void frame(int w, int h, int curfps) {
  const auto farplane = 100.f;
  const auto aspect = float(sys::scrw) / float(sys::scrh);
  const auto fovy = float(fov) / aspect;
  if (shadertoy)
    doshadertoy(fovy,aspect,farplane);
  else {
    context ctx(w,h,fov,aspect,farplane);
    ctx.begin();
    ctx.dogbuffer();
    ctx.dodeferred();
    ctx.dofxaa();
    ctx.end();
  }

  const auto hudtimer = ogl::begintimer("hud", true);
  ogl::disable(GL_CULL_FACE);
  drawhud(w,h,curfps);
  ogl::enable(GL_CULL_FACE);
  ogl::endtimer(hudtimer);
}
} /* namespace rr */
} /* namespace q */

