/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - polygon.hpp -> exposes 2d polygon triangulation
 -------------------------------------------------------------------------*/
#pragma once
#include "base/math.hpp"
#include "base/vector.hpp"
#include "base/list.hpp"

namespace q {
class polygon {
  vector<vec2f> points;
  bool hole;
public:
  polygon(int n=0);
  polygon(const vec2f&, const vec2f&, const vec2f&);
  polygon(const polygon&);
  polygon &operator=(const polygon&);
  INLINE long size() const {return points.size();}
  INLINE bool is_hole() const {return hole;}
  INLINE void set_hole(bool hole) {this->hole = hole;}
  INLINE vec2f operator[] (int i) const {return points[i];}
  INLINE vec2f &operator[] (int i) {return points[i];}
  void init(int n);
};
bool triangulate(const list<polygon>&, vector<vec2f>&);
} /* namespace q */

