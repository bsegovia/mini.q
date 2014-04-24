/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso.cpp -> implements routines for iso surface
 -------------------------------------------------------------------------*/
#pragma once
#include "csg.hpp"
#include "base/sys.hpp"
#include "base/math.hpp"

namespace q {
namespace iso {

// describe a set of consecutive primitives with same material
struct segment {u32 start, num, mat;};

// simple structure to describe meshes generated by marching cube or dual
// contouring
struct mesh {
  INLINE mesh(vec3f *pos=NULL, vec3f *nor=NULL, u32 *index=NULL,
              segment *seg=NULL, u32 vn=0, u32 idxn=0, u32 segn=0) :
    m_pos(pos), m_nor(nor), m_index(index), m_segment(seg),
    m_vertnum(vn), m_indexnum(idxn), m_segmentnum(segn) {}
  void destroy();
  vec3f *m_pos, *m_nor;
  u32 *m_index;
  segment *m_segment;
  u32 m_vertnum;
  u32 m_indexnum;
  u32 m_segmentnum;
};

// load/store the mesh in the given stream
void store(const char *filename, const mesh &m);
bool load(const char *filename, mesh &m);

// tesselate along a grid the distance field with dual contouring algorithm
mesh dc_mesh_mt(const vec3f &org, u32 cellnum, float cellsize, const csg::node &d);
void start();
void finish();
} /* namespace iso */
} /* namespace q */

