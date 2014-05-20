/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - polygon.cpp -> implements 2d polygon triangulation
 -------------------------------------------------------------------------*/
#include <cstdio>
#include <cstring>
#include <cmath>
#include "base/list.hpp"
#include "polygon.hpp"

namespace q {

polygon::polygon(int n) { init(n); }
polygon::polygon(const polygon &src) {
  hole = src.hole;
  points.resize(src.size());
  loopv(points) points[i] = src.points[i];
}
polygon &polygon::operator=(const polygon &src) {
  hole = src.hole;
  points.resize(src.size());
  loopv(points) points[i] = src.points[i];
  return *this;
}
void polygon::init(int n) {
  hole = false;
  points.resize(n);
}

INLINE vec2f safe_normalize(const vec2f &p) {
  const auto len2 = p.x*p.x + p.y*p.y;
  return len2!=0.f ? p/sqrt(len2) : vec2f(zero);
}

static bool intersects(const vec2f &p00, const vec2f &p01, const vec2f &p10, const vec2f &p11) {
  if (p00==p10 || p00 == p11 || p01 == p10 || p01 == p11) return false;
  const vec2f v0ort(p01.y-p00.y, p00.x-p01.x);
  const vec2f v1ort(p11.y-p10.y, p10.x-p11.x);
  const auto dot00 = dot(p00-p10, v1ort);
  const auto dot01 = dot(p01-p10, v1ort);
  const auto dot10 = dot(p10-p00, v0ort);
  const auto dot11 = dot(p11-p00, v0ort);
  if (dot00*dot01>0.f) return false;
  if (dot10*dot11>0.f) return false;
  return true;
}

INLINE bool is_convex(vec2f p0, vec2f p1, vec2f p2) {
  return (p2.y-p0.y)*(p1.x-p0.x)-(p2.x-p0.x)*(p1.y-p0.y) > 0.f;
}

static bool is_inside(const vec2f &p0, const vec2f &p1, const vec2f &p2, const vec2f &p) {
  if (is_convex(p0,p,p1) || is_convex(p1,p,p2) || is_convex(p2,p,p0))
    return false;
  return true;
}

static bool in_cone(const vec2f &p0, const vec2f &p1, const vec2f &p2, const vec2f &p) {
  if (is_convex(p0,p1,p2))
    return is_convex(p0,p1,p) && !is_convex(p1,p2,p);
  else
    return is_convex(p0,p1,p) || is_convex(p1,p2,p);
}

struct partition_vertex {
  partition_vertex *previous, *next;
  vec2f p;
  float angle;
  bool is_active, is_convex, is_ear;
};

// removes holes from in by merging them with non-holes
static bool remove_holes(const list<polygon> &in, list<polygon> &out) {
  list<polygon>::iterator holeiter,polyiter;
  int holeidx = -1,polyidx = -1;
  vec2f bestpolypt(zero);

  // check for trivial case (no holes)
  bool hasholes = false;
  for (auto iter = in.begin(); iter!=in.end(); ++iter)
    if (iter->is_hole()) {
      hasholes = true;
      break;
    }
  if (!hasholes) {
    for (auto iter = in.begin(); iter!=in.end(); ++iter)
      out.push_back(*iter);
    return true;
  }

  list<polygon> polys(in.begin(), in.end());
  for (;;) {
    // find the hole point with the largest x
    hasholes = false;
    for (auto iter = polys.begin(); iter!=polys.end(); ++iter) {
      if (!iter->is_hole()) continue;
      if (!hasholes) {
        hasholes = true;
        holeiter = iter;
        holeidx = 0;
      }
      loopi(iter->size()) if ((*iter)[i].x > (*holeiter)[holeidx].x) {
        holeiter = iter;
        holeidx = i;
      }
    }
    if (!hasholes) break;
    const auto holepoint = (*holeiter)[holeidx];

    bool pointfound = false;
    for (auto iter = polys.begin(); iter!=polys.end(); ++iter) {
      if (iter->is_hole()) continue;
      loopi(iter->size()) {
        const auto polypt = (*iter)[i];
        if (polypt.x <= holepoint.x) continue;
        const auto next = (*iter)[(i+1)%iter->size()];
        const auto prev = (*iter)[(i+iter->size()-1)%iter->size()];
        if (!in_cone(prev, polypt, next, holepoint))
          continue;
        if (pointfound) {
          const auto v0 = safe_normalize(polypt-holepoint);
          const auto v1 = safe_normalize(bestpolypt-holepoint);
          if (v1.x > v0.x) continue;
        }
        auto pointvisible = true;
        for (auto iter=polys.begin(); iter!=polys.end(); ++iter) {
          if (iter->is_hole()) continue;
          loopj(iter->size()) {
            const auto linep0 = (*iter)[j];
            const auto linep1 = (*iter)[(j+1)%iter->size()];
            if (intersects(holepoint,polypt,linep0,linep1)) {
              pointvisible = false;
              break;
            }
          }
          if (!pointvisible) break;
        }
        if (pointvisible) {
          pointfound = true;
          bestpolypt = polypt;
          polyiter = iter;
          polyidx = i;
        }
      }
    }

    if (!pointfound) return false;

    int j = 0;
    polygon newpoly(holeiter->size() + polyiter->size() + 2);
    loopi(polyidx+1) newpoly[j++] = (*polyiter)[i];
    loopi(holeiter->size()+1) newpoly[j++] = (*holeiter)[(i+holeidx)%holeiter->size()];
    rangei(polyidx, polyiter->size()) newpoly[j++] = (*polyiter)[i];

    polys.erase(holeiter);
    polys.erase(polyiter);
    polys.push_back(newpoly);
  }

  for (auto iter = polys.begin(); iter!=polys.end(); ++iter)
    out.push_back(*iter);
  return true;
}

static void update_vertex(partition_vertex *v, const vector<partition_vertex> &vert) {
  const auto v1 = v->previous;
  const auto v3 = v->next;
  const auto vec1 = safe_normalize(v1->p - v->p);
  const auto vec3 = safe_normalize(v3->p - v->p);
  v->angle = dot(vec1, vec3);
  v->is_convex = is_convex(v1->p,v->p,v3->p);

  if (v->is_convex) {
    v->is_ear = true;
    loopv(vert) {
      const auto &p = vert[i].p;
      if (p==v->p || p==v1->p || p==v3->p) continue;
      if (is_inside(v1->p,v->p,v3->p,p)) {
        v->is_ear = false;
        break;
      }
    }
  } else
    v->is_ear = false;
}

INLINE void add(vec2f p0, vec2f p1, vec2f p2, vector<vec2f> &tri) {
  tri.push_back(p0); tri.push_back(p1); tri.push_back(p2);
}

// triangulation by ear removal
static bool triangulate(const polygon &poly, vector<vec2f> &tri) {
  if (poly.size() < 3) return false;
  if (poly.size() == 3) {
    add(poly[0], poly[1], poly[2], tri);
    return true;
  }

  const int n = poly.size();
  vector<partition_vertex> vert(n);
  loopi(n) {
    vert[i].is_active = true;
    vert[i].p = poly[i];
    vert[i].next = &vert[i==(n-1) ? 0 : i+1];
    vert[i].previous = &vert[i==0 ? n-1 : i-1];
  }
  loopi(n) update_vertex(&vert[i],vert);

  loopi(n-3) {
    partition_vertex *ear = NULL;
    loopj(n) {
      if (!vert[j].is_active || !vert[j].is_ear) continue;
      if (ear == NULL)
        ear = &vert[j];
      else if (vert[j].angle > ear->angle)
        ear = &vert[j];
    }
    if (ear == NULL) return false;

    add(ear->previous->p, ear->p, ear->next->p, tri);
    ear->is_active = false;
    ear->previous->next = ear->next;
    ear->next->previous = ear->previous;
    if (i==n-4) break;

    update_vertex(ear->previous,vert);
    update_vertex(ear->next,vert);
  }
  loopi(n) if (vert[i].is_active) {
    add(vert[i].previous->p, vert[i].p, vert[i].next->p, tri);
    break;
  }
  return true;
}

bool triangulate(const list<polygon> &in, vector<vec2f> &tri) {
  list<polygon> out;
  if (!remove_holes(in, out)) return false;
  for (auto iter = out.begin(); iter != out.end(); ++iter)
    if (!triangulate(*iter, tri)) return false;
  return true;
}
} /* namespace q */

