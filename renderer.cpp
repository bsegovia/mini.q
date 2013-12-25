/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - renderer.cpp -> handles rendering routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "marching_cube.hpp"

namespace q {
namespace rr {

void drawdf() {
  ogl::pushmatrix();
  ogl::setortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
  const array<float,3> verts[] = {
    array<float,3>(-1.f, -1.f, 0.f),
    array<float,3>(1.f, -1.f, 0.f),
    array<float,3>(-1.f, 1.f, 0.f),
    array<float,3>(1.f, 1.f, 0.f),
  };
  ogl::disablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::bindshader(ogl::DFRM_SHADER);
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 0, 0, 4, &verts[0][0]);
  ogl::enablev(GL_CULL_FACE, GL_DEPTH_TEST);
  ogl::popmatrix();
}

/*--------------------------------------------------------------------------
 - handle the HUD (console, scores...)
 -------------------------------------------------------------------------*/
static void drawhud(int w, int h, int curfps) {
  auto cmd = con::curcmd();
  ogl::pushmode(ogl::MODELVIEW);
  ogl::identity();
  ogl::pushmode(ogl::PROJECTION);
  ogl::setortho(0.f, float(VIRTW), float(VIRTH), 0.f, -1.f, 1.f);
  ogl::enablev(GL_BLEND);
  OGL(DepthMask, GL_FALSE);
  if (cmd) text::drawf("> %s_", 20, 1570, cmd);
  ogl::popmode(ogl::PROJECTION);
  ogl::popmode(ogl::MODELVIEW);

  OGL(DepthMask, GL_TRUE);
  ogl::disable(GL_BLEND);
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
IVARP(showhudgun, 0, 1, 1);

static void drawhudmodel(int start, int end, float speed, int base) {
  md2::render(hudgunnames[game::player.gun], start, end,
              //game::player.o+vec3f(0.f,0.f,-10.f), game::player.ypr,
              game::player.o+vec3f(0.f,0.f,0.f), game::player.ypr+vec3f(0.f,0.f,0.f),
              //vec3f(zero), game::player.ypr+vec3f(90.f,0.f,0.f),
              false, 1.0f, speed, 0, base);
}

static void drawhudgun(float fovy, float aspect, float farplane) {
  if (!showhudgun) return;

  ogl::enablev(GL_CULL_FACE);
#if 0
  const int rtime = game::reloadtime(game::player.gunselect);
  if (game::player.lastaction &&
      game::player.lastattackgun==game::player.gunselect &&
      game::lastmillis()-game::player.lastaction<rtime)
    drawhudmodel(7, 18, rtime/18.0f, game::player.lastaction);
  else
#endif
  drawhudmodel(6, 1, 100.f, 0);
  ogl::disablev(GL_CULL_FACE);
}

/*--------------------------------------------------------------------------
 - render the complete frame
 -------------------------------------------------------------------------*/
FVARP(fov, 30.f, 90.f, 160.f);

static float dist_sphere(vec3f v, float r) { return length(v) - r; }

static const vec3f cubefverts[8] = {
  vec3f(0.f,0.f,0.f)/*0*/, vec3f(0.f,0.f,1.f)/*1*/, vec3f(0.f,1.f,1.f)/*2*/, vec3f(0.f,1.f,0.f)/*3*/,
  vec3f(1.f,0.f,0.f)/*4*/, vec3f(1.f,0.f,1.f)/*5*/, vec3f(1.f,1.f,1.f)/*6*/, vec3f(1.f,1.f,0.f)/*7*/
};
const vec3i icubev[8] = {
  vec3i(0,0,0)/*0*/, vec3i(0,0,1)/*1*/, vec3i(0,1,1)/*2*/, vec3i(0,1,0)/*3*/,
  vec3i(1,0,0)/*4*/, vec3i(1,0,1)/*5*/, vec3i(1,1,1)/*6*/, vec3i(1,1,0)/*7*/
};

#if 0
static void makescene() {
  triangle tri[16];
  u32 tot = 0;
  loopi(32) loopj(32) loopk(32) {
    gridcell cell;
    loop(l, 8) cell.p[l] = 0.10f * (cubefverts[l] + vec3f(float(i), float(j), float(k)));
    loop(l, 8) cell.val[l] = dist_sphere(cell.p[l]-vec3f(1.f,2.f,1.f), 1.f);
    const int n = tesselate(cell, tri);
    tot += n;
    loopi(n) loopj(3) scene_tris.add(tri[i].p[j]);
   // loopi(n) loopj(3) {printf("["); loopk(3) printf("%f ", tri[i].p[j][k]); printf("]\n");}
  }
  con::out("%d created triangles", tot);
}
#else
INLINE u32 index(vec3i xyz, vec3i dim) {
  return dim.x*dim.y*xyz.z+dim.x*xyz.y+xyz.x;
}

// define a vertex by the unique edge it belongs to
typedef vec3i vert[2];

// hash table using open addressing. It is allocated once in the constructor
// with an optional parameter to extend key space and avoid massive collision.
// fast and simple
template <typename Key, typename Value>
struct static_hash_table {
  static_hash_table(Key *k, u32 elem_n, u32 extend=2) :
    n(nextpowerof2(extend*elem_n)), table(n+elem_n), here(table.length()/32)
  {
    memset(&here[0], 0, here.size());
  }
  INLINE u32 ishere(u32 idx) const { return here[idx/32] & (1<<(idx%32)); }
  INLINE void sethere(u32 idx) { here[idx/32] |= 1<<(idx%32); }
  const Value &insert(const Key &k, const Value &v) {
    auto i = murmurhash2(k) & (n-1);
    const u32 elem_n = table.length();
    for (; i<elem_n; ++i) {
      if (ishere(i)) {
        if (table[i].first == k) return table[i].second;
      } else {
        sethere(i);
        table[i].first = k;
        return table[i].second = v;
      }
    }
    assert("unreachable: you must pre-allocate enough space");
    return v;
  }
  u32 n;
  vector<pair<Key, Value>> table;
  vector<u32> here;
};

static INLINE vec3f interp(vec3i p1, vec3i p2, float valp1, float valp2) {
  if (abs(valp1) < 0.00001f) return vec3f(p1);
  if (abs(valp2) < 0.00001f) return vec3f(p2);
  if (abs(valp1-valp2) < 0.00001f) return vec3f(p1);
  return vec3f(p1) - valp1 / (valp2 - valp1) * (vec3f(p2) - vec3f(p1));
}
static vector<u32> indexbuffer;
static vector<vec3f> vertexbuffer;

static void makescene() {
  auto scene = (float*) malloc(33*33*33*sizeof(float));
  const vec3i dim(33,33,33);
  loopi(dim.z) loopj(dim.y) loopk(dim.x) {
    const auto p = 0.10f * vec3f(float(k), float(j), float(i));
    const auto idx = index(vec3i(k,j,i), dim);
    scene[idx] = dist_sphere(p-vec3f(1.f,2.f,1.f), 1.f);
  }

  // generate all the triangles. we do not care about vertex duplication yet
  vector<pair<vec3i,vec3i>> tris;
  loopi(32) loopj(32) loopk(32) {
    mcell cell;
    vec3i xyz(k,j,i);
    loop(l,8) cell[l] = scene[index(icubev[l]+xyz, dim)];
    mvert v[64];
    const int n = tesselate(cell, v);
    loop(l,n) tris.add(makepair(xyz+icubev[v[l].x], xyz+icubev[v[l].y]));
  }

  // now generate the vertex buffer and the index buffer
  static_hash_table<pair<vec3i,vec3i>, u32> table(&tris[0], tris.length());
  loopv(tris) {
    const auto idx = table.insert(tris[i], vertexbuffer.length());
    if (idx == u32(vertexbuffer.length())) {
      const auto idx0 = index(tris[i].first, dim), idx1 = index(tris[i].second, dim);
      vertexbuffer.add(interp(tris[i].first, tris[i].second, scene[idx0], scene[idx1]));
    }
    indexbuffer.add(idx);
  }

  con::out("%d triangles and %d vertices", indexbuffer.length()/3, vertexbuffer.length());
  free(scene);
}

#endif

static void transplayer(void) {
  using namespace game;
  ogl::identity();
  ogl::rotate(player.ypr.z, vec3f(0.f,0.f,1.f));
  ogl::rotate(player.ypr.y, vec3f(1.f,0.f,0.f));
  ogl::rotate(player.ypr.x, vec3f(0.f,1.f,0.f));
  ogl::translate(-player.o);
}

void frame(int w, int h, int curfps) {
  const float farplane = 100.f;
  const float fovy = fov * float(sys::scrh) / float(sys::scrw);
  const float aspect = float(sys::scrw) / float(sys::scrh);
  OGL(ClearColor, 0.f, 0.f, 0.f, 1.f);
  OGL(Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ogl::matrixmode(ogl::PROJECTION);
  ogl::setperspective(fovy, aspect, 0.15f, farplane);
  ogl::matrixmode(ogl::MODELVIEW);
  transplayer();

  const float verts[] = {
    -100.f, -100.f, -100.f, 0.f, -100.f,
    +100.f, -100.f, +100.f, 0.f, -100.f,
    -100.f, +100.f, -100.f, 0.f, +100.f,
    +100.f, +100.f, +100.f, 0.f, +100.f
  };
  ogl::bindfixedshader(ogl::DIFFUSETEX);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHECKBOARD));
  ogl::immdraw(GL_TRIANGLE_STRIP, 3, 2, 0, 4, verts);

  if (indexbuffer.length() == 0) makescene();
#if 0
  ogl::enable(GL_CULL_FACE);
  ogl::bindfixedshader(0);
  ogl::immdraw(GL_TRIANGLES, 3, 0, 0, scene_tris.length(), &scene_tris[0][0]);
#endif

  drawhud(w,h,0);
  drawhudgun(fovy, aspect, farplane);
}
} /* namespace rr */
} /* namespace q */

