/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - iso.cpp -> implements routines for iso surface
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "stl.hpp"
#include "math.hpp"

namespace q {
namespace iso {

// simple structure to describe meshes generated by marching cube or dual
// contouring
struct mesh {
  INLINE mesh(vec3f *pos=NULL, vec3f *nor=NULL, u32 *index=NULL, u32 vn=0, u32 idxn=0) :
    m_pos(pos), m_nor(nor), m_index(index), m_vertnum(vn), m_indexnum(idxn) {}
  ~mesh();
  vec3f *m_pos, *m_nor;
  u32 *m_index;
  u32 m_vertnum;
  u32 m_indexnum;
};

// describe a grid with a given location in the world
struct grid {
  INLINE grid(const vec3f &cellsize = vec3f(one),
              const vec3f &org = vec3f(zero),
              const vec3i &dim = vec3i(one)) :
    m_cellsize(cellsize), m_org(org), m_dim(dim) {}
  INLINE vec3f vertex(const vec3i &p) {return m_org+m_cellsize*vec3f(p);}
  vec3f m_cellsize;
  vec3f m_org;
  vec3i m_dim;
};

// get the distance to the field from 'pos'
typedef float (*distance_field)(const vec3f &pos);

// estimate the gradient (i.e. normal vector) at the given position
static const float DEFAULT_GRAD_STEP = 1e-3f;
vec3f gradient(distance_field d, const vec3f &pos, float grad_step = DEFAULT_GRAD_STEP);

// tesselate along a grid the distance field with dual contouring algorithm
mesh dc_mesh(const grid &grid, distance_field f);

mesh dc_mesh(const vec3f &org, u32 cellnum, float cellsize, distance_field f);
} /* namespace iso */
} /* namespace q */

