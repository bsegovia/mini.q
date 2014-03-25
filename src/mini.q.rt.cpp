/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - mini.q.rt.cpp -> test ray tracing routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"
#include "bvh.hpp"
#include "rt.hpp"
#include <zlib.h>

namespace q {
VARP(fov, 30, 70, 160);
void finish() {}
static void playerpos(int x, int y, int z) {game::player1->o = vec3f(vec3i(x,y,z));}
static void playerypr(int x, int y, int z) {game::player1->ypr = vec3f(vec3i(x,y,z));}
CMD(playerpos, ARG_3INT);
CMD(playerypr, ARG_3INT);
static void run(int argc, const char *argv[]) {
  con::out("init: memory debugger");
  sys::memstart();
  con::out("init: tasking system");
  const u32 threadnum = sys::threadnumber() - 1;
  con::out("init: tasking system: %d threads created", threadnum);
  task::start(&threadnum, 1);
  con::out("init: isosurface module");
  iso::start();

  // create mesh data
#if 0
  const float start = sys::millis();
  const auto node = csg::makescene();
  const auto m = iso::dc_mesh_mt(vec3f(zero), 2*8192, 0.1f, *node);
  const auto duration = sys::millis() - start;
  con::out("csg: tris %i verts %i", m.m_indexnum/3, m.m_vertnum);
  con::out("csg: elapsed %f ms ", float(duration));
#else
  iso::mesh m;
  con::out("init: loading san-miguel.ogz");
  const auto start = sys::millis();
  auto f = gzopen("san-miguel.ogz", "rb");
  gzread(f, &m.m_indexnum, sizeof(m.m_indexnum));
  gzread(f, &m.m_vertnum, sizeof(m.m_vertnum));
  gzread(f, &m.m_segmentnum, sizeof(m.m_segmentnum));
  m.m_index = (u32*) MALLOC(sizeof(u32)*m.m_indexnum);
  m.m_pos = (vec3f*) MALLOC(sizeof(vec3f)*m.m_vertnum);
  m.m_nor = (vec3f*) MALLOC(sizeof(vec3f)*m.m_vertnum);
  m.m_segment = (iso::segment*) MALLOC(sizeof(iso::segment)*m.m_segmentnum);
  gzread(f, m.m_index, sizeof(u32) * m.m_indexnum);
  gzread(f, m.m_pos, sizeof(vec3f) * m.m_vertnum);
  gzread(f, m.m_nor, sizeof(vec3f) * m.m_vertnum);
  gzread(f, m.m_segment, sizeof(iso::segment) * m.m_segmentnum);
  gzclose(f);
#endif
  con::out("init: san-miguel.ogz loaded in %.2f ms", float(sys::millis()-start));

  // create bvh
  rt::buildbvh(m.m_pos, m.m_index, m.m_indexnum);

  // load position
  script::execfile(argv[1]);
  const auto pos = game::player1->o;
  const auto ypr = game::player1->ypr;
  //rt::raytrace(argv[2], pos, ypr, 1920/4, 1080/4, fov, 1.f);
  loopi(4) rt::raytrace(argv[2], pos, ypr, 1920, 1080, fov, 1.f);
}
} /* namespace q */

int main(int argc, const char *argv[]) {
  if (argc != 3) {
    q::con::out("usage: %s script outname", argv[0]);
    return 1;
  }
  q::run(argc, argv);
  return 0;
}

