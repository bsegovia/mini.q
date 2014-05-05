/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - geom.cpp -> implements mesh creation, simplification and update routines
 -------------------------------------------------------------------------*/
#include "geom.hpp"
#include "qef.hpp"
#include "iso.hpp"
#include "base/task.hpp"
#include "base/vector.hpp"
#include "base/console.hpp"

namespace q {
namespace geom {

template <typename T>
INLINE bool isdegenerated(T a, T b, T c) { return a==b || a==c || b==c; }

void mesh::destroy() {
  if (m_pos) {FREE(m_pos); m_pos=NULL;}
  if (m_nor) {FREE(m_nor); m_nor=NULL;}
  if (m_index) {FREE(m_index); m_index=NULL;}
  if (m_segment) {FREE(m_segment); m_segment=NULL;}
}

// error below which we merge vertices
static const double QEM_MIN_ERROR = 1e-8;

// beyond this length, we do not merge edges
static const float MAX_EDGE_LEN = 4.f;

// below this, we make the edge sharp
static const float SHARP_EDGE_THRESHOLD = 0.2f;

// edges that we estimate too small are decimated automatically
static const float MIN_EDGE_FACTOR = 1.f/8.f;

// number of iterations of decimations
static const u32 DECIMATION_NUM = 2;

// we have to choose between this two meshes and take the one that does not self
// intersect
struct quadmesh { int tri[2][3]; };
static const quadmesh qmesh[2] = {{{{0,1,2},{0,2,3}}},{{{0,1,3},{3,1,2}}}};

// test first configuration for non self intersection. If ok, take it, otherwise
// take the other one
static INLINE const quadmesh &findbestmesh(iso::octree::qefpoint **pt) {
  const auto qm0 = qmesh[0].tri;
  const auto e00 = pt[qm0[0][0]]->pos-pt[qm0[0][1]]->pos;
  const auto e01 = pt[qm0[0][0]]->pos-pt[qm0[0][2]]->pos;
  const auto n0 = cross(e00,e01);
  const auto e10 = pt[qm0[1][0]]->pos-pt[qm0[1][1]]->pos;
  const auto e11 = pt[qm0[1][0]]->pos-pt[qm0[1][2]]->pos;
  const auto n1 = cross(e10,e11);
  return dot(n0,n1) > 0.f ? qmesh[0] : qmesh[1];
}

/*-------------------------------------------------------------------------
 - build a regular "to-process" mesh from the qef points and quads
 -------------------------------------------------------------------------*/
struct procmesh {
  vector<vec3f> pos, nor;
  vector<u32> idx, mat;
  vector<int> vidx;
  vector<pair<int,int>> vtri;
  INLINE int trinum() const {return idx.length()/3;}
};

static void buildmesh(const iso::octree &o, const iso::octree::node &node, procmesh &pm) {
  if (!node.isleaf) {
    loopi(8) buildmesh(o, node.children[i], pm);
    return;
  } else if (node.leaf == NULL)
    return;

#if DEBUGOCTREE
  bool missingpoint = false;
#endif /* DEBUGOCTREE */

  loopv(node.leaf->quads) {
    // get four points
    const auto &q = node.leaf->quads[i];
    const auto quadmat = q.matindex;
    iso::octree::qefpoint *pt[4];
    loopk(4) {
      const auto ipos = vec3i(q.index[k]) + node.org;
      const auto leaf = o.findleaf(ipos);

#if DEBUGOCTREE
      if (leaf == NULL || leaf->leaf == NULL) {
        missingpoint = true;
        break;
      }
#else
      assert(leaf != NULL && leaf->leaf != NULL &&
        "leaf node is missing from the octree");
#endif /* DEBUGOCTREE */

      const auto vidx = ipos % vec3i(iso::SUBGRID);
      const auto qef = leaf->leaf->get(vidx);
      assert(qef != NULL && "point is missing from leaf octree");
      pt[k] = qef;
    }

#if DEBUGOCTREE
    if (missingpoint)
      continue;
#endif /* DEBUGOCTREE */

    // get the right convex configuration
    const auto tri = findbestmesh(pt).tri;

    // append positions in the vertex buffer and the index buffer
    loopk(2) {
      const auto t = tri[k];
      if (isdegenerated(pt[t[0]],pt[t[1]],pt[t[2]]))
        continue;
      pm.mat.add(quadmat);
      loopl(3) {
        const auto qef = pt[t[l]];
        if (qef->idx == -1) {
          qef->idx = pm.pos.length();
          pm.pos.add(qef->pos);
        }
        pm.idx.add(qef->idx);
      }
    }
  }
}

/*-------------------------------------------------------------------------
 - run mesh decimation on a regular "to-process" mesh
 -------------------------------------------------------------------------*/
struct qemedge {
  INLINE qemedge() {}
  INLINE qemedge(int i0, int i1, int mat) : mat(mat), best(0), num(1) {
    idx[0] = i0;
    idx[1] = i1;
    timestamp[0] = timestamp[1] = 0;
  }
  int idx[2];
  int timestamp[2];
  int mat;
  int best:1;
  int num:31;
};

struct qemheapitem {
  double cost;
  float len2;
  int idx;
};

INLINE bool operator< (const qemheapitem &i0, const qemheapitem &i1) {
  if (i0.cost == 0.0 && i1.cost == 0.0)
    return i0.len2 < i1.len2;
  else
    return i0.cost < i1.cost;
}

struct qemcontext {
  vector<int> vidx;          // list of triangle per vertex
  vector<pair<int,int>> vtri;// <trinum, firstidx> triangle list per vertex
  vector<qef::qem> vqem;     // qem per vertex
  vector<qemedge> eqem;      // qem information per edge
  vector<qemheapitem> heap;  // heap to decimate the mesh
  vector<int> mergelist;     // temporary structure when merging triangle lists
};

static void extraplane(const procmesh &pm, const qemedge &edge, int tri,
                       qef::qem &q0, qef::qem &q1)
{
  // need to find which vertex is the third one
  const auto t = &pm.idx[3*tri];
  const auto i0 = edge.idx[0], i1 = edge.idx[1];
  int i2 = t[2];
  loopi(2) if (int(t[i])!=i0 && int(t[i])!=i1) i2=t[i];

  // now we build plane that contains the edge and that is perpendicular to the
  // triangle
  const auto &p = pm.pos;
  const auto n0 = cross(p[i2]-p[i0], p[i1]-p[i0]);
  const auto d = cross(n0, p[i1]-p[i0]);
  const auto len2 = length2(d);
  if (len2 != 0.f) {
    const auto q = qef::qem(d*rsqrt(len2),p[i0]);
    q0 += q;
    q1 += q;
  }
}

static void buildedges(qemcontext &ctx, const procmesh &pm) {
  auto &e = ctx.eqem;
  auto &v = ctx.vqem;

  // maintain a list of edges per triangle
  const auto vertnum = pm.pos.length(), trinum = pm.trinum();
  vector<int> vlist(pm.pos.length()+pm.idx.length());
  loopi(vertnum) vlist[i] = -1;
  int firstedge = vertnum;

  // first append all edges with i0 < i1
  loopi(trinum) {
    const auto mat = pm.mat[i];
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 < i1) {
        vlist[firstedge++] = vlist[i0];
        vlist[i0] = e.length();
        e.add(qemedge(i0,i1,mat));
      }
      i0 = i1;
    }
  }

  // with i1 < i0, we now handle border and multi-material edges
  auto nextedge = &vlist[0] + vertnum;
  loopi(trinum) {
    const auto mat = pm.mat[i];
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 > i1) {
        int idx = vlist[i1];
        for (; idx != -1; idx = nextedge[idx]) {
          const auto &edge = e[idx];
          if (edge.idx[1] == i0) break;
        }

        // border edge
        if (idx == -1) {
          vlist[firstedge++] = vlist[i1];
          vlist[i1] = e.length();
          e.add(qemedge(i1,i0,mat));
          extraplane(pm, e.last(), i, v[i0], v[i1]);
        } else {
          auto &edge = e[idx];
          ++edge.num;

          // is it a multi-material edge? if so, add a plane to separate both
          // materials
          if (u32(edge.mat) != mat) extraplane(pm, edge, i, v[i0], v[i1]);
        }
      }
      i0 = i1;
    }
  }

  // loop again over all triangles and add planes for border edges with i0 < i1
  loopi(trinum) {
    const auto t = &pm.idx[3*i];
    int i0 = t[2];
    loopj(3) {
      const int i1 = t[j];
      if (i0 < i1) {
        int idx = vlist[i0];
        for (; idx != -1; idx = nextedge[idx]) {
          const auto &edge = e[idx];
          if (edge.idx[1] == i1) break;
        }
        assert(idx != -1 && e[idx].num >= 1);
        if (e[idx].num == 1) extraplane(pm, e[idx], i, v[i0], v[i1]);
      }
      i0 = i1;
    }
  }
}

static void buildheap(qemcontext &ctx, procmesh &pm) {
  auto &h = ctx.heap;
  auto &e = ctx.eqem;
  auto &v = ctx.vqem;
  auto &p = pm.pos;
  loopv(e) {
    const auto idx0 = e[i].idx[0], idx1 = e[i].idx[1];
    const auto &p0 = p[idx0], &p1 = p[idx1];
    const auto &q0 = v[idx0], &q1 = v[idx1];
    const auto best = qef::findbest(q0,q1,p0,p1,QEM_MIN_ERROR);
    if (best.first > QEM_MIN_ERROR)
      continue;
    e[i].best = best.second;
    h.add({best.first,distance2(p0,p1),i});
  }
  h.buildheap();
}

INLINE int uncollapsedidx(vector<qef::qem> &v, int idx) {
  while (v[idx].next != -1) idx = v[idx].next;
  return idx;
}

static bool merge(qemcontext &ctx, procmesh &pm, const qemedge &edge, int idx0, int idx1) {

  // first we gather non degenerated triangles from both vertex triangle lists
  ctx.mergelist.setsize(0);
  const int idx[] = {idx0,idx1}, other[] = {idx1,idx0};
  int pivot;
  loopi(2) {
    const auto first = ctx.vtri[idx[i]].first;
    const auto n = ctx.vtri[idx[i]].second;
    pivot = ctx.mergelist.length();
    loopj(n) {
      const auto tri = ctx.vidx[first+j];
      const auto i0 = pm.idx[3*tri], i1 = pm.idx[3*tri+1], i2 = pm.idx[3*tri+2];
      if (int(i0) == other[i] || int(i1) == other[i] || int(i2) == other[i])
        continue;
      ctx.mergelist.add(tri);
    }
  }

  // now we need to know if this is going to flip normals. if so, the merge is
  // invalid
  const auto from = edge.best == 0 ? idx1 : idx0;
  const auto to = edge.best == 0 ? idx0 : idx1;
  const auto begin = edge.best == 0 ? pivot : 0;
  const auto end = edge.best == 0 ? ctx.mergelist.length() : pivot;
  rangei(begin,end) {
    const auto &p = pm.pos;
    const auto t = &pm.idx[3*ctx.mergelist[i]];
    auto i0 = int(t[0]), i1 = int(t[1]), i2 = int(t[2]);
    const vec3f initial(cross(p[i0]-p[i1],p[i0]-p[i2]));
    i0 = i0 == from ? to : i0;
    i1 = i1 == from ? to : i1;
    i2 = i2 == from ? to : i2;
    if (isdegenerated(i0,i1,i2)) continue;
    const vec3f target(cross(p[i0]-p[i1],p[i0]-p[i2]));
    if (dot(initial,target) <= 0.f)
      return false;
  }

  // we are good to go. we replace each 'from' by 'to' in the index buffer
  loopi(2) {
    const auto first = ctx.vtri[idx[i]].first;
    const auto n = ctx.vtri[idx[i]].second;
    loopj(n) {
      const auto tri = ctx.vidx[first+j];
      const auto t = &pm.idx[3*tri];
      loopk(3) if (int(t[k]) == from) t[k] = to;
    }
  }

  // we now commit the merged triangle list. we try to see if we can reuse one
  // of both lists. if not, we just linear allocate a new range
  // XXX we should remove degenerated triangles as well
  int first = -1;
  loopi(2) if (ctx.vtri[idx[i]].second >= ctx.mergelist.length()) {
    first = ctx.vtri[idx[i]].first;
    break;
  }
  if (first == -1) {
    first = ctx.vidx.length();
    ctx.vidx.setsize(first+ctx.mergelist.length());
  }
  loopv(ctx.mergelist) ctx.vidx[first+i] = ctx.mergelist[i];
  ctx.vtri[to].first = first;
  ctx.vtri[to].second = ctx.mergelist.length();

  // add both qems
  auto &q0 = ctx.vqem[idx0], &q1 = ctx.vqem[idx1];
  const auto timestamp0 = q0.timestamp, timestamp1 = q1.timestamp;
  const auto q = q0+q1;
  const auto newstamp = max(timestamp0, timestamp1);
  assert(timestamp0==edge.timestamp[0] && timestamp1==edge.timestamp[1]);
  if (edge.best == 0) {
    q0 = q;
    q1.next = idx0;
  } else {
    q1 = q;
    q0.next = idx1;
  }
  q0.timestamp = q1.timestamp = newstamp+1;
  return true;
}

static void decimatemesh(qemcontext &ctx, procmesh &pm, float edgeminlen) {
  auto &heap = ctx.heap;
  auto &vqem = ctx.vqem;
  auto &eqem = ctx.eqem;
  bool anychange = true;

  // first we just remove all edges smaller than the given threshold. we iterate
  // until there is nothing left to remove
  while (anychange) {
    anychange = false;
    loopv(eqem) {
      auto &edge = eqem[i];
      const auto idx0 = uncollapsedidx(vqem, edge.idx[0]);
      const auto idx1 = uncollapsedidx(vqem, edge.idx[1]);
      const auto &p0 = pm.pos[idx0];
      const auto &p1 = pm.pos[idx1];

      // we do not care about this edge if already folded or too big
      if (idx0 == idx1 || distance(p0,p1) >= edgeminlen) continue;

      // need to update it properly here
      auto &q0 = vqem[idx0], &q1 = vqem[idx1];
      if (q0.timestamp != edge.timestamp[0] || q1.timestamp != edge.timestamp[1]) {
        edge.best = qef::findbest(q0,q1,p0,p1,QEM_MIN_ERROR).second;
        edge.timestamp[0] = q0.timestamp;
        edge.timestamp[1] = q1.timestamp;
      }

      // now we can safely merge it
      anychange = merge(ctx, pm, edge, idx0, idx1) || anychange;
    }
  }

  // we remove zero cost edges
  for (;;) {
    const auto item = heap.removeheap();
    if (item.len2 > MAX_EDGE_LEN*MAX_EDGE_LEN) continue;
    auto &edge = eqem[item.idx];
    const auto idx0 = uncollapsedidx(vqem, edge.idx[0]);
    const auto idx1 = uncollapsedidx(vqem, edge.idx[1]);

    // already removed. we ignore it
    if (idx0 == idx1) continue;

    // try to remove it. the cost function needs to be properly updated
    const auto &p0 = pm.pos[idx0], &p1 = pm.pos[idx1];
    auto &q0 = vqem[idx0], &q1 = vqem[idx1];
    assert(q0.timestamp >= edge.timestamp[0]);
    assert(q1.timestamp >= edge.timestamp[1]);

    // edge is upto date. we can safely remove it
    const auto timestamp0 = q0.timestamp, timestamp1 = q1.timestamp;
    if (timestamp0 == edge.timestamp[0] && timestamp1 == edge.timestamp[1]) {
      if (item.cost > QEM_MIN_ERROR) break;
      merge(ctx, pm, edge, idx0, idx1);
      continue;
    }

    // edge is too old. we need to update its cost and reinsert it
    const auto best = qef::findbest(q0,q1,p0,p1,QEM_MIN_ERROR);
    const qemheapitem newitem = {best.first, distance2(p0,p1), item.idx};
    edge.best = best.second;
    edge.timestamp[0] = timestamp0;
    edge.timestamp[1] = timestamp1;
    edge.idx[0] = idx0;
    edge.idx[1] = idx1;
    heap.addheap(newitem);
  }

  // now, we remove unused vertices and degenerated triangles
  vector<int> mapping(pm.pos.length());
  loopv(mapping) mapping[i] = -1;
  vector<u32> newidx, newmat;
  const auto trinum = pm.trinum();
  auto vertnum = 0;
  loopi(trinum) {
    const u32 idx[] = {pm.idx[3*i+0], pm.idx[3*i+1], pm.idx[3*i+2]};
    if (idx[0] == idx[1] || idx[1] == idx[2] || idx[2] == idx[0])
      continue;
    newmat.add(pm.mat[i]);
    loopj(3) {
      if (mapping[idx[j]] == -1) mapping[idx[j]] = vertnum++;
      newidx.add(mapping[idx[j]]);
    }
  }
  newidx.moveto(pm.idx);
  newmat.moveto(pm.mat);

  // compact vertex buffer
  vector<vec3f> newpos(vertnum);
  loopv(mapping) if (mapping[i] != -1) newpos[mapping[i]] = pm.pos[i];
  newpos.moveto(pm.pos);
}

static void buildqem(qemcontext &ctx, procmesh &pm) {
  auto &v = ctx.vqem;
  v.setsize(pm.pos.length());
  loopi(pm.trinum()) {
    const auto i0 = pm.idx[3*i+0];
    const auto i1 = pm.idx[3*i+1];
    const auto i2 = pm.idx[3*i+2];
    const auto v0 = pm.pos[i0];
    const auto v1 = pm.pos[i1];
    const auto v2 = pm.pos[i2];
    const qef::qem q(v0,v1,v2);
    v[i0] += q;
    v[i1] += q;
    v[i2] += q;
  }
}

static void buildtrianglelists(qemcontext &ctx, const procmesh &pm) {
  auto &vtri = ctx.vtri;
  auto &vidx = ctx.vidx;
  vtri.setsize(pm.pos.length());
  vidx.setsize(pm.idx.length());

  // first initialize tri lists
  loopv(vtri) vtri[i].first = vtri[i].second = 0;
  loopv(pm.idx) ++vtri[pm.idx[i]].second;
  auto accum = 0;
  loopv(vtri) {
    vtri[i].first = accum;
    accum += vtri[i].second;
    vtri[i].second = 0;
  }
  assert(accum == pm.idx.length());

  // then, fill them
  loopv(pm.idx) {
    auto &v = vtri[pm.idx[i]];
    vidx[v.first+v.second++] = i/3;
  }
}

static void decimatemesh(procmesh &pm, float cellsize) {
  if (pm.idx.length() == 0) return;
  qemcontext ctx;

  // we go over all triangles and build all vertex qem
  buildqem(ctx, pm);

  // build the list of edges. append extra planes when needed (border and
  // multi-material edges)
  buildedges(ctx, pm);

  // evaluate the cost for each edge and build the heap
  buildheap(ctx, pm);

  // generate the lists of triangles per-vertex
  buildtrianglelists(ctx, pm);

  // decimate the mesh using quadric error functions
  const auto minlen = cellsize*MIN_EDGE_FACTOR;
  decimatemesh(ctx, pm, minlen);
}

/*-------------------------------------------------------------------------
 - sharpen mesh i.e. duplicate sharp points and compute vertex normals
 -------------------------------------------------------------------------*/
static void sharpenmesh(procmesh &pm) {
  if (pm.idx.length() == 0) return;
  const auto trinum = pm.trinum();
  vector<int> vertlist(pm.pos.length());
  vector<vec3f> newnor(pm.pos.length());
  vector<u32> newidx, newmat;

  // init the chain list
  loopv(vertlist) vertlist[i] = i;

#if 1 //!defined(NDEBUG)
  loopv(newnor) newnor[i] = vec3f(zero);
#endif

  // go over all triangles and initialize vertex normals
  loopi(trinum) {
    const auto t = &pm.idx[3*i];
    const auto edge0 = pm.pos[t[2]]-pm.pos[t[0]];
    const auto edge1 = pm.pos[t[2]]-pm.pos[t[1]];

    // zero sized edge are not possible since we got rid of them...
    assert(length(edge0) != 0.f && length(edge1) != 0.f);

    // ...but colinear edges are still possible
    const auto dir = cross(edge0, edge1);
    const auto len2 = length2(dir);
    if (len2 == 0.f) continue;
    const auto nor = dir*rsqrt(len2);
    loopj(3) {
      auto idx = int(t[j]);
      if (vertlist[idx] == idx) {
        vertlist[idx] = -1;
        newnor[idx] = dir;
      } else {
        while (idx != -1) {
          const auto cosangle = dot(nor, normalize(newnor[idx]));
          if (cosangle > SHARP_EDGE_THRESHOLD) break;
          idx = vertlist[idx];
        }
        if (idx != -1)
          newnor[idx] += dir;
        else {
          const auto old = vertlist[t[j]];
          idx = newnor.length();
          vertlist[t[j]] = newnor.length();
          vertlist.add(old);
          pm.pos.add(pm.pos[t[j]]);
          newnor.add(dir);
        }
      }
      newidx.add(idx);
    }
    newmat.add(pm.mat[i]);
  }

  // renormalize all normals
  loopv(newnor) {
    const auto len2 = length2(newnor[i]);
    if (len2 != 0.f) newnor[i] = newnor[i]*rsqrt(len2);
  }
  newnor.moveto(pm.nor);
  newidx.moveto(pm.idx);
  newmat.moveto(pm.mat);
}

/*-------------------------------------------------------------------------
 - build a final mesh from the qef points and quads stored in the octree
 -------------------------------------------------------------------------*/

// build a first mesh from contoured octree
struct isomeshtask : public task {
  INLINE isomeshtask(iso::octree &o, procmesh &pm) :
    task("isomeshtask"), o(o), pm(pm)
  {}
  virtual void run(u32) {
    buildmesh(o, o.m_root, pm);
    con::out("iso: procmesh: %d vertices", pm.pos.length());
    con::out("iso: procmesh: %d triangles", pm.idx.length()/3);
  }
  iso::octree &o;
  procmesh &pm;
};

// decimate a procmesh using qem
struct decimatetask : public task {
  INLINE decimatetask(procmesh &pm, float cellsize) :
    task("decimatetask"), pm(pm), cellsize(cellsize)
  {}
  virtual void run(u32) { decimatemesh(pm, cellsize); }
  procmesh &pm;
  float cellsize;
};

// create proper (possible sharpened) normals and finish the mesh
struct finishtask : public task {
  INLINE finishtask(mesh &m, procmesh &pm) :
    task("finishtask"), m(m), pm(pm)
  {}
  virtual void run(u32) {
    // handle sharp edges and create normals
    sharpenmesh(pm);

    // build the segment list
    vector<segment> seg;
    u32 currmat = ~0x0;
    loopv(pm.mat) {
      if (pm.mat[i] != currmat) {
        seg.add({3u*i,0u,pm.mat[i]});
        currmat = pm.mat[i];
      }
      seg.last().num += 3;
    }

#if !defined(NDEBUG)
    loopv(pm.pos) assert(!isnan(pm.pos[i].x)&&!isnan(pm.pos[i].y)&&!isnan(pm.pos[i].z));
    loopv(pm.pos) assert(!isinf(pm.pos[i].x)&&!isinf(pm.pos[i].y)&&!isinf(pm.pos[i].z));
    loopv(pm.nor) assert(!isnan(pm.nor[i].x)&&!isnan(pm.nor[i].y)&&!isnan(pm.nor[i].z));
    loopv(pm.nor) assert(!isinf(pm.nor[i].x)&&!isinf(pm.nor[i].y)&&!isinf(pm.nor[i].z));
#endif /* !defined(NDEBUG) */

    const auto p = pm.pos.move();
    const auto n = pm.nor.move();
    const auto idx = pm.idx.move();
    const auto s = seg.move();
    con::out("iso: final: %d vertices", p.second);
    con::out("iso: final: %d triangles", idx.second/3);
    m.init(p.first, n.first, idx.first, s.first, p.second, idx.second, s.second);
  }

  mesh &m;
  procmesh &pm;
};

// task to build the mesh from a "contoured" octree
struct meshbuildtask : public task {
  INLINE meshbuildtask(mesh &m, iso::octree &o, float cellsize, int waiternum) :
    task("meshbuildtask", 1, waiternum), m(m), o(o), cellsize(cellsize)
  {}

  virtual void run(u32) {
    // create all tasks needed for the mesh processing
    ref<task> init = NEW(isomeshtask, o, pm);
    ref<task> decimate[DECIMATION_NUM];
    loopi(DECIMATION_NUM) decimate[i] = NEW(decimatetask, pm, cellsize);
    ref<task> finish = NEW(finishtask, m, pm);

    // handle dependencies and completion of parent task
    init->starts(*decimate[0]);
    rangei(1,DECIMATION_NUM) decimate[i-1]->starts(*decimate[i]);
    decimate[DECIMATION_NUM-1]->starts(*finish);
    finish->ends(*this);

    // schedule everything
    finish->scheduled();
    loopi(DECIMATION_NUM) decimate[i]->scheduled();
    init->scheduled();
  }

  mesh &m;
  iso::octree &o;
  float cellsize;
  procmesh pm;
};

ref<task> buildmesh(mesh &m, iso::octree &o, float cellsize, int waiternum) {
  return NEW(meshbuildtask, m, o, cellsize, waiternum);
}

/*-------------------------------------------------------------------------
 - mesh interface (very simple)
 -------------------------------------------------------------------------*/
mesh::mesh() {ZERO(this);}

void mesh::init(vec3f *pos, vec3f *nor, u32 *index,
                segment *seg, u32 vn, u32 idxn, u32 segn) {
  m_pos = pos;
  m_nor = nor;
  m_index = index;
  m_segment = seg;
  m_vertnum = vn;
  m_indexnum = idxn;
  m_segmentnum = segn;
}

void store(const char *filename, const mesh &m) {
  auto f = fopen(filename, "wb");
  assert(f);
  fwrite(&m.m_vertnum, sizeof(u32), 1, f);
  fwrite(&m.m_indexnum, sizeof(u32), 1, f);
  fwrite(&m.m_segmentnum, sizeof(u32), 1, f);
  fwrite(m.m_pos, sizeof(vec3f) * m.m_vertnum, 1, f);
  fwrite(m.m_nor, sizeof(vec3f) * m.m_vertnum, 1, f);
  fwrite(m.m_index, sizeof(u32) * m.m_indexnum, 1, f);
  fwrite(m.m_segment, sizeof(segment) * m.m_segmentnum, 1, f);
  fclose(f);
}

bool load(const char *filename, mesh &m) {
  auto f = fopen(filename, "rb");
  if (f==NULL) return false;
  m.destroy();
  fread(&m.m_vertnum, sizeof(u32), 1, f);
  fread(&m.m_indexnum, sizeof(u32), 1, f);
  fread(&m.m_segmentnum, sizeof(u32), 1, f);
  m.m_pos = (vec3f*) MALLOC(sizeof(vec3f) * m.m_vertnum);
  m.m_nor = (vec3f*) MALLOC(sizeof(vec3f) * m.m_vertnum);
  m.m_index = (u32*) MALLOC(sizeof(u32) * m.m_indexnum);
  m.m_segment = (segment*) MALLOC(sizeof(segment) * m.m_segmentnum);
  fread(m.m_pos, sizeof(vec3f) * m.m_vertnum, 1, f);
  fread(m.m_nor, sizeof(vec3f) * m.m_vertnum, 1, f);
  fread(m.m_index, sizeof(u32) * m.m_indexnum, 1, f);
  fread(m.m_segment, sizeof(segment) * m.m_segmentnum, 1, f);
  fclose(f);
  return true;
}
} /* namespace geom */
} /* namespace q */

