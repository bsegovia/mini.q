/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer fps
 - obj.cpp -> load Maya .obj files
 -------------------------------------------------------------------------*/
#include "obj.hpp"
#include "base/sys.hpp"

#include <zlib.h>
#include <map>// XXX replace it// XXX replace it
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace q {
namespace {
struct objloader {
  static const int FILE_NAME_SZ = 1024;
  static const int MAT_NAME_SZ = 1024;
  static const int LINE_SZ = 4096;
  static const int MAX_VERT_NUM = 4;
  INLINE objloader(void) : objsaved(NULL), mtlsaved(NULL) {}
  bool loadobj(const char *filename);
  bool loadmtl(const char *mtlfilename, const char *objfilename);

  // face as parsed by the OBJ loader
  struct face {
    int vertexindex[MAX_VERT_NUM];
    int normalindex[MAX_VERT_NUM];
    int textureindex[MAX_VERT_NUM];
    int vertexnum;
    int materialindex;
  };

  // material as parsed by the OBJ loader
  struct mat {
    void setdefault(void);
    char name[MAT_NAME_SZ];
    char mapka[FILE_NAME_SZ];
    char mapkd[FILE_NAME_SZ];
    char mapd[FILE_NAME_SZ];
    char mapbump[FILE_NAME_SZ];
    vec3d amb, diff, spec;
    double km;
    double reflect;
    double refract;
    double trans;
    double shiny;
    double glossy;
    double refract_index;
  };

  int findmaterial(const char *name);
  face *parseface(void);
  vec3d *parsevector(void);
  void parsevertexindex(objloader::face &face);
  INLINE int getlistindex(int currMax, int index) {
    if (index == 0) return -1;
    if (index < 0)  return currMax + index;
    return index - 1;
  }
  void getlistindex(int current_max, int *indices) {
    loopi(MAX_VERT_NUM) indices[i] = getlistindex(current_max, indices[i]);
  }
  INLINE face *allocface(void) { return faceallocator.allocate(); }
  INLINE vec3d *allocvec(void) { return vectorallocator.allocate(); }
  INLINE mat *allocmat(void)   { return matallocator.allocate(); }
  vector<vec3d*> vertexlist, normallist, texturelist;
  vector<face*> facelist;
  vector<mat*> materiallist;
  growingpool<vec3d> vectorallocator;
  growingpool<face> faceallocator;
  growingpool<mat> matallocator;
  char *objsaved, *mtlsaved; // for tokenize(...)
  static const char *whitespace;
};
const char *objloader::whitespace = " \t\n\r";

void objloader::mat::setdefault(void) {
  mapka[0] = mapkd[0] = mapd[0] = mapbump[0] = '\0';
  amb = vec3d(0.2);
  diff = vec3d(0.8);
  spec = vec3d(1.0);
  shiny = reflect = 0.0;
  refract_index = trans = 1.;
  glossy = 98.;
}

void objloader::parsevertexindex(objloader::face &face) {
  auto saved = &objsaved;
  char *temp_str = NULL, *token = NULL;
  int vertexnum = 0;

  while ((token = tokenize(NULL, whitespace, saved)) != NULL) {
    if (face.textureindex != NULL) face.textureindex[vertexnum] = 0;
    if (face.normalindex != NULL)  face.normalindex[vertexnum] = 0;
    face.vertexindex[vertexnum] = atoi(token);

    if (contains(token, "//")) {
      temp_str = strchr(token, '/');
      temp_str++;
      face.normalindex[vertexnum] = atoi(++temp_str);
    } else if (contains(token, "/")) {
      temp_str = strchr(token, '/');
      face.textureindex[vertexnum] = atoi(++temp_str);
      if (contains(temp_str, "/")) {
        temp_str = strchr(temp_str, '/');
        face.normalindex[vertexnum] = atoi(++temp_str);
      }
    }
    vertexnum++;
  }
  face.vertexnum = vertexnum;
}

int objloader::findmaterial(const char *name) {
  loopv(materiallist) if (strequal(name, materiallist[i]->name)) return i;
  return -1;
}

objloader::face *objloader::parseface(void) {
  auto face = allocface();
  parsevertexindex(*face);
  getlistindex(vertexlist.length(), face->vertexindex);
  getlistindex(texturelist.length(), face->textureindex);
  getlistindex(normallist.length(), face->normalindex);
  return face;
}

vec3d *objloader::parsevector(void) {
  auto v = allocvec();
  *v = zero;
  loopi(3) {
    auto str = tokenize(NULL, whitespace, &objsaved);
    if (str == NULL) break;
    (*v)[i] = atof(str);
  }
  return v;
}

bool objloader::loadmtl(const char *mtlfilename, const char *objfilename) {
  char *tok = NULL;
  FILE *mtlfile = NULL;
  mat *currmat = NULL;
  char currline[LINE_SZ];
  int lineno = 0;
  char material_open = 0;

  if ((mtlfile = fopen(mtlfilename, "r")) == NULL) return false;
  while (fgets(currline, LINE_SZ, mtlfile)) {
    auto saved = &mtlsaved;
    tok = tokenize(currline, " \t\n\r", saved);
    lineno++;

    if (tok == NULL || strequal(tok, "//") || strequal(tok, "#")) // skip comments
      continue;
    else if (strequal(tok, "newmtl")) { // start material
      material_open = 1;
      currmat = allocmat();
      currmat->setdefault();
      strncpy(currmat->name, tokenize(NULL, whitespace, saved), MAT_NAME_SZ);
      materiallist.add(currmat);
    } else if (strequal(tok, "Ka") && material_open) // ambient
      loopi(3) currmat->amb[i] = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "Kd") && material_open) // diff
      loopi(3) currmat->diff[i] = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "Ks") && material_open) // specular
      loopi(3) currmat->spec[i] = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "Ns") && material_open) // shiny
      currmat->shiny = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "Km") && material_open) // mapKm
      currmat->km = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "d") && material_open) // transparent
      currmat->trans = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "r") && material_open) // reflection
      currmat->reflect = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "sharpness") && material_open) // glossy
      currmat->glossy = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "Ni") && material_open) // refract index
      currmat->refract_index = atof(tokenize(NULL, " \t", saved));
    else if (strequal(tok, "map_Ka") && material_open) // mapka
      strncpy(currmat->mapka, tokenize(NULL, " \"\t\r\n", saved), FILE_NAME_SZ);
    else if (strequal(tok, "map_Kd") && material_open) // mapkd
      strncpy(currmat->mapkd, tokenize(NULL, " \"\t\r\n", saved), FILE_NAME_SZ);
    else if (strequal(tok, "map_D") && material_open) // mapd
      strncpy(currmat->mapd, tokenize(NULL, " \"\t\r\n", saved), FILE_NAME_SZ);
    else if (strequal(tok, "map_Bump") && material_open) // mapbump
      strncpy(currmat->mapbump, tokenize(NULL, " \"\t\r\n", saved), FILE_NAME_SZ);
    else if (strequal(tok, "illum") && material_open) {} // illumination type
    else printf("obj: unknown command : %s in material file %s at line %i, \"%s\"\n",
                      tok, mtlfilename, lineno, currline);
  }

  fclose(mtlfile);
  return true;
}

bool objloader::loadobj(const char *filename) {
  FILE *objfile = NULL;
  int currmaterial = -1; 
  char *tok = NULL;
  char currline[LINE_SZ];
  int lineno = 0;

  // open scene
  objfile = fopen(filename, "r");
  if (objfile == NULL) {
    printf("obj: error reading file: %s\n", filename);
    return false;
  }

  // parser loop
  while (fgets(currline, LINE_SZ, objfile)) {
    auto saved = &objsaved;
    tok = tokenize(currline, " \t\n\r", saved);
    lineno++;

    // skip comments
    if (tok == NULL || tok[0] == '#') continue;
    // parse objects
    else if (strequal(tok, "v")) // vertex
      vertexlist.add(parsevector());
    else if (strequal(tok, "vn")) // vertex normal
      normallist.add(parsevector());
    else if (strequal(tok, "vt")) // vertex texture
      texturelist.add(parsevector());
    else if (strequal(tok, "f")) { // face
      auto face = parseface();
      face->materialindex = currmaterial;
      facelist.add(face);
    } else if (strequal(tok, "usemtl"))   // usemtl
      currmaterial = findmaterial(tokenize(NULL, whitespace, saved));
    else if (strequal(tok, "mtllib")) { //  mtllib
      const char *mtlfilename = tokenize(NULL, whitespace, saved);
      if (loadmtl(mtlfilename, filename) == 0) {
        printf("obj: error loading %s\n", mtlfilename);
        return false;
      }
      continue;
    } else if (strequal(tok, "p")) {} // point
    else if (strequal(tok, "o")) {} // object name
    else if (strequal(tok, "s")) {} // smoothing
    else if (strequal(tok, "g")) {} // group
    else printf("obj: unknown command: %s in obj file %s at line %i, \"%s\"\n",
                tok, filename, lineno, currline);
  }

  fclose(objfile);
  return true;
}

static INLINE bool cmp(obj::triangle t0, obj::triangle t1) {return t0.m < t1.m;}

// sort vertex faces
struct vertexkey {
  INLINE vertexkey(void) {}
  INLINE vertexkey(int p, int n, int t) : p(p), n(n), t(t) {}
  bool operator== (const vertexkey &other) const {
    return p==other.p && n==other.n && t==other.t;
  }
  bool operator< (const vertexkey &other) const {
    if (p != other.p) return p < other.p;
    if (n != other.n) return n < other.n;
    if (t != other.t) return t < other.t;
    return false;
  }
  int p,n,t;
};

// store face connectivity
struct poly { int v[4]; int mat; int n; };
} // namespace

bool obj::load(const char *filename) {
  objloader loader;
  std::map<vertexkey, int> map;
  vector<poly> polys;
  if (loader.loadobj(filename) == 0) return false;

  int vert_n = 0;
  loopv(loader.facelist) {
    const auto face = loader.facelist[i];
    if (face == NULL) {
      printf("obj: null face for %s\n", filename);
      return false;
    }
    if (face->vertexnum > 4) {
      printf("obj: too many vertices for %s\n", filename);
      return false;
    }
    int v[] = {0,0,0,0};
    for (int j = 0; j < face->vertexnum; ++j) {
      const int p = face->vertexindex[j];
      const int n = face->normalindex[j];
      const int t = face->textureindex[j];
      const vertexkey key(p,n,t);
      const auto it = map.find(key);
      if (it == map.end())
        v[j] = map[key] = vert_n++;
      else
        v[j] = it->second;
    }
    const poly p = {{v[0],v[1],v[2],v[3]},face->materialindex,face->vertexnum};
    polys.add(p);
  }

  // No face defined
  if (polys.length() == 0) return true;

  // create triangles now
  vector<triangle> tris;
  for (auto poly = polys.begin(); poly != polys.end(); ++poly) {
    if (poly->n == 3) {
      const triangle tri(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
      tris.add(tri);
    } else {
      const triangle tri0(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
      const triangle tri1(vec3i(poly->v[0], poly->v[2], poly->v[3]), poly->mat);
      tris.add(tri0);
      tris.add(tri1);
    }
  }

  // sort triangle by material and create the material groups
  quicksort(tris.begin(), tris.end(), cmp);
  vector<matgroup> matgrp;
  int curr = tris[0].m;
  matgrp.add(matgroup(0,0,curr));
  loopv(tris) if (tris[i].m != curr) {
    curr = tris[i].m;
    matgrp.last().last = (int) (i-1);
    matgrp.add(matgroup((int)i,0,curr));
  }
  matgrp.last().last = tris.length() - 1;

  // we replace the undefined material by the default one if needed
  if (tris[0].m == -1) {
    auto mat = loader.allocmat();
    mat->setdefault();
    loader.materiallist.add(mat);
    const auto matindex = loader.materiallist.length() - 1;
    loopv(tris)
      if (tris[i].m != -1)
        break;
      else
        tris[i].m = matindex;
    assert(matgrp[0].m == -1);
    matgrp[0].m = matindex;
  }

  // create all the vertices and store them
  vector<vertex> verts(map.size());
  bool allposset = true, allnorset = true, alltexset = true;
  for (auto it = map.begin(); it != map.end(); ++it) {
    const auto src = it->first;
    const auto dst = it->second;
    vertex &v = verts[dst];

    // get the position if specified
    if (src.p != -1)
      v.p = vec3f(*loader.vertexlist[src.p]);
    else {
      v.p = zero;
      allposset = false;
    }

    // get the normal if specified
    if (src.n != -1)
      v.n = vec3f(*loader.normallist[src.n]);
    else {
      v.n = zero;
      allnorset = false;
    }

    // get the texture coordinates if specified
    if (src.t != -1) {
      const auto t = loader.texturelist[src.t];
      v.t[0] = float((*t)[0]);
      v.t[1] = float((*t)[1]);
    } else {
      v.t = zero;
      alltexset = false;
    }
  }

  if (!allposset) printf("obj: some positions are unspecified for %s\n", filename);
  if (!allnorset) printf("obj: some normals are unspecified for %s\n", filename);
  if (!alltexset) printf("obj: some texture coordinates are unspecified for %s\n", filename);

  auto matarray = NEWAE(material, loader.materiallist.length());
  memset(matarray, 0, sizeof(material) * loader.materiallist.length());

#define COPY_FIELD(NAME)\
if (from.NAME) {\
  const size_t len = strlen(from.NAME);\
  to.NAME = NEWAE(char, len + 1);\
  if (len) memcpy(to.NAME, from.NAME, len);\
  to.NAME[len] = 0;\
}
  loopv(loader.materiallist) {
    const auto &from = *loader.materiallist[i];
    assert(loader.materiallist[i] != NULL);
    auto &to = matarray[i];
    COPY_FIELD(name);
    COPY_FIELD(mapka);
    COPY_FIELD(mapkd);
    COPY_FIELD(mapd);
    COPY_FIELD(mapbump);
  }
#undef COPY_FIELD

  // now return the properly allocated obj
  memset(this, 0, sizeof(obj));
  trinum = tris.length();
  vertnum = verts.length();
  grpnum = matgrp.length();
  matnum = loader.materiallist.length();
  if (trinum) {
    tri = NEWAE(triangle, trinum);
    memcpy(tri, &tris[0],  sizeof(triangle) * trinum);
  }
  if (vertnum) {
    vert = NEWAE(vertex, vertnum);
    memcpy(vert, &verts[0], sizeof(vertex) * vertnum);
  }
  if (grpnum) {
    grp = NEWAE(matgroup, grpnum);
    memcpy(grp,  &matgrp[0],  sizeof(matgroup) * grpnum);
  }
  mat = matarray;
  printf("obj: %s, %i triangles\n", filename, trinum);
  printf("obj: %s, %i vertices\n", filename, vertnum);
  printf("obj: %s, %i groups\n", filename, grpnum);
  printf("obj: %s, %i materials\n", filename, matnum);
  return true;
}

obj::obj(void) { memset(this, 0, sizeof(obj)); }
obj::~obj(void) {
  SAFE_DELA(tri);
  SAFE_DELA(vert);
  SAFE_DELA(grp);
  loopi(int(matnum)) {
    SAFE_DELA(mat[i].name);
    SAFE_DELA(mat[i].mapka);
    SAFE_DELA(mat[i].mapkd);
    SAFE_DELA(mat[i].mapd);
    SAFE_DELA(mat[i].mapbump);
  }
  SAFE_DELA(mat);
}
void finish() {}
struct segment {u32 start, num, mat;};
} // namespace q

int main(int argc, const char *argv[]) {
  using namespace q;
  if (argc != 3) {
    printf("usage: %s objname binary\n", argv[0]);
    return 1;
  }

  // load the obj file first
  const auto start = sys::millis();
  printf("obj: loading %s\n", argv[1]);
  obj o;
  const auto loaded = o.load(argv[1]);
  if (!loaded) {
    printf("obj: failed to load %s\n", argv[1]);
    return 1;
  }

  // convert everything as needed
  auto seg = NEWAE(segment, o.grpnum);
  loopi(int(o.grpnum)) {
    seg[i].start = o.grp[i].first*3;
    seg[i].num = (o.grp[i].last-o.grp[i].first+1)*3;
    seg[i].mat = 0;
  }
  auto pos = NEWAE(vec3f, o.vertnum);
  auto nor = NEWAE(vec3f, o.vertnum);
  loopi(int(o.vertnum)) {
    pos[i] = o.vert[i].p;
    nor[i] = o.vert[i].n;
  }
  auto idx = NEWAE(int, o.trinum*3);
  loopi(int(o.trinum)) {
    idx[3*i+0] = o.tri[i].v.x;
    idx[3*i+1] = o.tri[i].v.y;
    idx[3*i+2] = o.tri[i].v.z;
  }

  // export the mesh into a binary file
  printf("obj: exporting %s\n", argv[2]);
  const auto f = gzopen(argv[2], "wb");
  gzwrite(f, &o.trinum, sizeof(o.trinum));
  gzwrite(f, &o.vertnum, sizeof(o.vertnum));
  gzwrite(f, &o.grpnum, sizeof(o.grpnum));
  gzwrite(f, idx, sizeof(int) * o.trinum*3);
  gzwrite(f, pos, sizeof(vec3f) * o.vertnum);
  gzwrite(f, nor, sizeof(vec3f) * o.vertnum);
  gzwrite(f, seg, sizeof(segment) * o.grpnum);
  gzclose(f);
  DELA(seg);
  DELA(pos);
  DELA(nor);
  DELA(idx);
  const auto ms = sys::millis()-start;
  printf("obj: elapsed: %.2f ms\n", float(ms));
  return 0;
}

