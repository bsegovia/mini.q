/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - md2.cpp -> handles quake md2 models
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {

#define SHADERNAME md2
#define VERTEX_PROGRAM "data/shaders/md2_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/md2_fp.decl"
#include "shaderdecl.hxx"

namespace md2 {

struct header {
  int magic;
  int version;
  int skinwidth, skinheight;
  int framesz;
  int numskins, numvertices, numtexcoords;
  int trianglenum, numglcommands, numframes;
  int offsetskins, offsettexcoords, offsettriangles;
  int offsetframes, offsetglcommands, offsetend;
};

struct vertex { u8 vertex[3], normalidx; };
static const float normaltable[256][3] = {
  {-0.525731f,  0.000000f,  0.850651f}, {-0.442863f,  0.238856f,  0.864188f}, {-0.295242f,  0.000000f,  0.955423f}, {-0.309017f,  0.500000f,  0.809017f}, 
  {-0.162460f,  0.262866f,  0.951056f}, { 0.000000f,  0.000000f,  1.000000f}, { 0.000000f,  0.850651f,  0.525731f}, {-0.147621f,  0.716567f,  0.681718f}, 
  { 0.147621f,  0.716567f,  0.681718f}, { 0.000000f,  0.525731f,  0.850651f}, { 0.309017f,  0.500000f,  0.809017f}, { 0.525731f,  0.000000f,  0.850651f}, 
  { 0.295242f,  0.000000f,  0.955423f}, { 0.442863f,  0.238856f,  0.864188f}, { 0.162460f,  0.262866f,  0.951056f}, {-0.681718f,  0.147621f,  0.716567f}, 
  {-0.809017f,  0.309017f,  0.500000f}, {-0.587785f,  0.425325f,  0.688191f}, {-0.850651f,  0.525731f,  0.000000f}, {-0.864188f,  0.442863f,  0.238856f}, 
  {-0.716567f,  0.681718f,  0.147621f}, {-0.688191f,  0.587785f,  0.425325f}, {-0.500000f,  0.809017f,  0.309017f}, {-0.238856f,  0.864188f,  0.442863f}, 
  {-0.425325f,  0.688191f,  0.587785f}, {-0.716567f,  0.681718f, -0.147621f}, {-0.500000f,  0.809017f, -0.309017f}, {-0.525731f,  0.850651f,  0.000000f}, 
  { 0.000000f,  0.850651f, -0.525731f}, {-0.238856f,  0.864188f, -0.442863f}, { 0.000000f,  0.955423f, -0.295242f}, {-0.262866f,  0.951056f, -0.162460f}, 
  { 0.000000f,  1.000000f,  0.000000f}, { 0.000000f,  0.955423f,  0.295242f}, {-0.262866f,  0.951056f,  0.162460f}, { 0.238856f,  0.864188f,  0.442863f}, 
  { 0.262866f,  0.951056f,  0.162460f}, { 0.500000f,  0.809017f,  0.309017f}, { 0.238856f,  0.864188f, -0.442863f}, { 0.262866f,  0.951056f, -0.162460f}, 
  { 0.500000f,  0.809017f, -0.309017f}, { 0.850651f,  0.525731f,  0.000000f}, { 0.716567f,  0.681718f,  0.147621f}, { 0.716567f,  0.681718f, -0.147621f}, 
  { 0.525731f,  0.850651f,  0.000000f}, { 0.425325f,  0.688191f,  0.587785f}, { 0.864188f,  0.442863f,  0.238856f}, { 0.688191f,  0.587785f,  0.425325f}, 
  { 0.809017f,  0.309017f,  0.500000f}, { 0.681718f,  0.147621f,  0.716567f}, { 0.587785f,  0.425325f,  0.688191f}, { 0.955423f,  0.295242f,  0.000000f}, 
  { 1.000000f,  0.000000f,  0.000000f}, { 0.951056f,  0.162460f,  0.262866f}, { 0.850651f, -0.525731f,  0.000000f}, { 0.955423f, -0.295242f,  0.000000f}, 
  { 0.864188f, -0.442863f,  0.238856f}, { 0.951056f, -0.162460f,  0.262866f}, { 0.809017f, -0.309017f,  0.500000f}, { 0.681718f, -0.147621f,  0.716567f}, 
  { 0.850651f,  0.000000f,  0.525731f}, { 0.864188f,  0.442863f, -0.238856f}, { 0.809017f,  0.309017f, -0.500000f}, { 0.951056f,  0.162460f, -0.262866f}, 
  { 0.525731f,  0.000000f, -0.850651f}, { 0.681718f,  0.147621f, -0.716567f}, { 0.681718f, -0.147621f, -0.716567f}, { 0.850651f,  0.000000f, -0.525731f}, 
  { 0.809017f, -0.309017f, -0.500000f}, { 0.864188f, -0.442863f, -0.238856f}, { 0.951056f, -0.162460f, -0.262866f}, { 0.147621f,  0.716567f, -0.681718f}, 
  { 0.309017f,  0.500000f, -0.809017f}, { 0.425325f,  0.688191f, -0.587785f}, { 0.442863f,  0.238856f, -0.864188f}, { 0.587785f,  0.425325f, -0.688191f}, 
  { 0.688191f,  0.587785f, -0.425325f}, {-0.147621f,  0.716567f, -0.681718f}, {-0.309017f,  0.500000f, -0.809017f}, { 0.000000f,  0.525731f, -0.850651f}, 
  {-0.525731f,  0.000000f, -0.850651f}, {-0.442863f,  0.238856f, -0.864188f}, {-0.295242f,  0.000000f, -0.955423f}, {-0.162460f,  0.262866f, -0.951056f}, 
  { 0.000000f,  0.000000f, -1.000000f}, { 0.295242f,  0.000000f, -0.955423f}, { 0.162460f,  0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
  {-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, { 0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
  { 0.147621f, -0.716567f, -0.681718f}, { 0.000000f, -0.525731f, -0.850651f}, { 0.309017f, -0.500000f, -0.809017f}, { 0.442863f, -0.238856f, -0.864188f}, 
  { 0.162460f, -0.262866f, -0.951056f}, { 0.238856f, -0.864188f, -0.442863f}, { 0.500000f, -0.809017f, -0.309017f}, { 0.425325f, -0.688191f, -0.587785f}, 
  { 0.716567f, -0.681718f, -0.147621f}, { 0.688191f, -0.587785f, -0.425325f}, { 0.587785f, -0.425325f, -0.688191f}, { 0.000000f, -0.955423f, -0.295242f}, 
  { 0.000000f, -1.000000f,  0.000000f}, { 0.262866f, -0.951056f, -0.162460f}, { 0.000000f, -0.850651f,  0.525731f}, { 0.000000f, -0.955423f,  0.295242f}, 
  { 0.238856f, -0.864188f,  0.442863f}, { 0.262866f, -0.951056f,  0.162460f}, { 0.500000f, -0.809017f,  0.309017f}, { 0.716567f, -0.681718f,  0.147621f}, 
  { 0.525731f, -0.850651f,  0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, {-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
  {-0.850651f, -0.525731f,  0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, {-0.716567f, -0.681718f,  0.147621f}, {-0.525731f, -0.850651f,  0.000000f}, 
  {-0.500000f, -0.809017f,  0.309017f}, {-0.238856f, -0.864188f,  0.442863f}, {-0.262866f, -0.951056f,  0.162460f}, {-0.864188f, -0.442863f,  0.238856f}, 
  {-0.809017f, -0.309017f,  0.500000f}, {-0.688191f, -0.587785f,  0.425325f}, {-0.681718f, -0.147621f,  0.716567f}, {-0.442863f, -0.238856f,  0.864188f}, 
  {-0.587785f, -0.425325f,  0.688191f}, {-0.309017f, -0.500000f,  0.809017f}, {-0.147621f, -0.716567f,  0.681718f}, {-0.425325f, -0.688191f,  0.587785f}, 
  {-0.162460f, -0.262866f,  0.951056f}, { 0.442863f, -0.238856f,  0.864188f}, { 0.162460f, -0.262866f,  0.951056f}, { 0.309017f, -0.500000f,  0.809017f}, 
  { 0.147621f, -0.716567f,  0.681718f}, { 0.000000f, -0.525731f,  0.850651f}, { 0.425325f, -0.688191f,  0.587785f}, { 0.587785f, -0.425325f,  0.688191f}, 
  { 0.688191f, -0.587785f,  0.425325f}, {-0.955423f,  0.295242f,  0.000000f}, {-0.951056f,  0.162460f,  0.262866f}, {-1.000000f,  0.000000f,  0.000000f}, 
  {-0.850651f,  0.000000f,  0.525731f}, {-0.955423f, -0.295242f,  0.000000f}, {-0.951056f, -0.162460f,  0.262866f}, {-0.864188f,  0.442863f, -0.238856f}, 
  {-0.951056f,  0.162460f, -0.262866f}, {-0.809017f,  0.309017f, -0.500000f}, {-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
  {-0.809017f, -0.309017f, -0.500000f}, {-0.681718f,  0.147621f, -0.716567f}, {-0.681718f, -0.147621f, -0.716567f}, {-0.850651f,  0.000000f, -0.525731f}, 
  {-0.688191f,  0.587785f, -0.425325f}, {-0.587785f,  0.425325f, -0.688191f}, {-0.425325f,  0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
  {-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

struct frame {
  float scale[3];
  float translate[3];
  char name[16];
  vertex vertices[1];
};

struct mdl {
  mdl(void) { memset(this,0,sizeof(mdl)); }
  ~mdl(void) {
    if (vbo) ogl::deletebuffers(1, &vbo);
    if (tex) ogl::deletetextures(1, &tex);
    FREE(loadname);
  }
  typedef array<float,8> vertextype;
  bool load(const char *filename, float scale, int snap);
  void render(int frame, int range, const vec3f &o, const vec3f &ypr,
             float scale, float speed, int snap, float basetime);
  u32 vbo, tex;
  u32 vboframesz;
  game::mapmodelinfo mmi;
  char *loadname;
  u32 mdlnum:31;
  u32 loaded:1;
};

static INLINE float snap(int sn, float f) {
  return sn ? (float)(((int)(f+sn*0.5f))&(~(sn-1))) : f;
}

bool mdl::load(const char *name, float scale, int sn) {
  FILE *file;
  header header;

  if ((file=fopen(name, "rb"))==NULL) return false;

  fread(&header, sizeof(header), 1, file);
  sys::endianswap(&header, sizeof(int), sizeof(header)/sizeof(int));

  if (header.magic != 844121161 || header.version != 8) return false;
  auto frames = NEWAE(char,header.framesz*header.numframes);

  fseek(file, header.offsetframes, SEEK_SET);
  fread(frames, header.framesz*header.numframes, 1, file);
  loopi(header.numframes) sys::endianswap(frames+i*header.framesz,sizeof(float), 6);

  auto glcommands = NEWAE(int, header.numglcommands);
  if (glcommands==NULL) return false;
  fseek(file, header.offsetglcommands, SEEK_SET);
  fread(glcommands, header.numglcommands*sizeof(int), 1, file);
  sys::endianswap(glcommands, sizeof(int), header.numglcommands);

  con::out("md2: '%s' with %d frames of %d bytes", name, header.numframes, header.framesz);
  fclose(file);

  // allocate one vbo for all key frames together
  for (int *command=glcommands, i=0; (*command)!=0; ++i) {
    const int n = abs(*command++);
    vboframesz += 3*(n-2)*sizeof(vertextype);
    command += 3*n; // +1 for index, +2 for u,v
  }
  ogl::genbuffers(1, &vbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  OGL(BufferData, GL_ARRAY_BUFFER, header.numframes*vboframesz, NULL, GL_STATIC_DRAW);

#if 0
  m *= mat3x3f::rotate(180.f, vec3f(1.f,0.f,0.f));
#endif
  const auto m = mat3x3f::rotate(90.f, vec3f(0.f,1.f,0.f));

  // insert each frame in the vbo
  loopj(header.numframes) {
    const auto cf = (const md2::frame*) (frames+header.framesz*j);
    const auto sc = 16.0f/scale;

    vector<vertextype> tris;
    for (int *command = glcommands, i=0; (*command)!=0; ++i) {
      const int moden = *command++;
      const int n = abs(moden);
      vector<vertextype> trisv;
      loopi(n) {
        const auto s = *((const float*)command++);
        const auto t = *((const float*)command++);
        const auto vn = *command++;
        const auto cv = (u8*) &cf->vertices[vn].vertex;
        const vec3f v((snap(sn,cv[0]*cf->scale[0])+cf->translate[0])/sc,
                      -(snap(sn,cv[1]*cf->scale[1])+cf->translate[1])/sc,
                      (snap(sn,cv[2]*cf->scale[2])+cf->translate[2])/sc);
        const auto nptr = normaltable[cf->vertices[vn].normalidx];
        const auto n = vec3f(nptr[0],-nptr[1],nptr[2]);
        trisv.add(vertextype(s,t,m*n.xzy(),m*v.xzy()));
      }
      loopi(n-2) { // just stolen from cube. TODO use an index buffer
        if (moden <= 0) { // fan
          tris.add(trisv[0]);
          tris.add(trisv[i+1]);
          tris.add(trisv[i+2]);
        } else // strip
          loopk(3) tris.add(trisv[i&1 && k ? i+(1-(k-1))+1 : i+k]);
      }
    }
    OGL(BufferSubData, GL_ARRAY_BUFFER, vboframesz*j, vboframesz, &tris[0][0]);
  }
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  SAFE_DELA(frames);
  SAFE_DELA(glcommands);
  return true;
}

void mdl::render(int frame, int range, const vec3f &o,
                 const vec3f &ypr, float sc, float speed,
                 int snap, float basetime)
{
  frame = 0;
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  ogl::pushmatrix();
  ogl::identity();
  const vec3f tr(10.310894f, 1.800000f, 13.364042f);
  auto m = mat3x3f::rotate(-ypr.x, vec3f(0.f,1.f,0.f));
  m *= mat3x3f::rotate(-ypr.y, vec3f(1.f,0.f,0.f));
  m *= mat3x3f::rotate(-ypr.z, vec3f(0.f,0.f,1.f));
  //ogl::rotate(180.f, vec3f(1.f,0.f,0.f));
//  ogl::rotate(90.f, vec3f(0.f,1.f,0.f));
#if 0
  auto m = mat3x3f::rotate(-ypr.x, vec3f(0.f,1.f,0.f));
  m *= mat3x3f::rotate(-ypr.y, vec3f(1.f,0.f,0.f));
  m *= mat3x3f::rotate(-ypr.z+180.f, vec3f(0.f,0.f,1.f));
  m *= mat3x3f::rotate(90.f, vec3f(0.f,1.f,0.f));
#elif 1
//  ogl::identity();
#else
#endif
#if 0
  auto m = mat3x3f::rotate(-ypr.x, vec3f(0.f,1.f,0.f));
  m *= mat3x3f::rotate(-ypr.y, vec3f(1.f,0.f,0.f));
  m *= mat3x3f::rotate(-ypr.z, vec3f(0.f,0.f,1.f));
  m *= mat3x3f::rotate(90.f, vec3f(0.f,1.f,0.f));
#else
  //auto m = mat3x3f(one);
#endif
 // ogl::mulmatrix(mat4x4f(m));

  const int n = vboframesz / sizeof(vertextype);
  const auto time = game::lastmillis-basetime;
  auto fr1 = intptr_t(time/speed);
  const auto frac = (time-fr1*speed)/speed;
  fr1 = fr1%range+frame;
  auto fr2 = fr1+1;
  if (fr2>=frame+range) fr2 = frame;
  const auto pos0 = (const float*)(fr1*vboframesz);
  const auto pos1 = (const float*)(fr2*vboframesz);
  OGL(CullFace, GL_FRONT);
  OGL(VertexAttribPointer, ogl::ATTRIB_TEX0, 2, GL_FLOAT, 0, sizeof(vertextype), pos0);
  OGL(VertexAttribPointer, ogl::ATTRIB_NOR0, 3, GL_FLOAT, 0, sizeof(vertextype), pos0+2);
  OGL(VertexAttribPointer, ogl::ATTRIB_NOR1, 3, GL_FLOAT, 0, sizeof(vertextype), pos1+2);
  OGL(VertexAttribPointer, ogl::ATTRIB_POS0, 3, GL_FLOAT, 0, sizeof(vertextype), pos0+5);
  OGL(VertexAttribPointer, ogl::ATTRIB_POS1, 3, GL_FLOAT, 0, sizeof(vertextype), pos1+5);
  ogl::setattribarray()(ogl::ATTRIB_POS0, ogl::ATTRIB_POS1,
                        ogl::ATTRIB_TEX0, ogl::ATTRIB_NOR0,
                        ogl::ATTRIB_NOR1);
  ogl::bindtexture(GL_TEXTURE_2D, tex, 0);
  ogl::bindshader(s);
  OGL(Uniform1f, s.u_delta, frac);
  const auto &mv = ogl::matrix(ogl::MODELVIEW);
  const auto &p = ogl::matrix(ogl::PROJECTION);
  const auto mvp = p*mv;
  OGL(UniformMatrix3fv, s.u_nortransform, 1, GL_FALSE, &m.vx.x);
  OGL(UniformMatrix4fv, s.u_mvp, 1, GL_FALSE, &mvp.vx.x);
  ogl::drawarrays(GL_TRIANGLES, 0, n);
  ogl::disableattribarray(ogl::ATTRIB_NOR0);
  ogl::disableattribarray(ogl::ATTRIB_NOR1);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::popmatrix();
  OGL(CullFace, GL_BACK);
}

static vector<mdl*> mapmodels;
static hashtable<mdl*> mdllookup;
static int modelnum = 0;

static void delayedload(mdl *m, float scale, int snap) {
  if (!m->loaded) {
    sprintf_sd(mdlpath)("data/models/%s/tris.md2", m->loadname);
    if (!m->load(sys::path(mdlpath), scale, snap))
      sys::fatal("loadmodel: ", mdlpath);
    sprintf_sd(texpath)("data/models/%s/skin.jpg", m->loadname);
    m->tex = ogl::installtex(texpath);
    m->loaded = 1;
  }
}

void start() {}
#if !defined(RELEASE)
void finish() {
  for (auto &m : mdllookup) {
    DEL(m.second);
    mdllookup.remove(&m);
  }
}
#endif

mdl *loadmodel(const char *name) {
  auto mm = mdllookup.access(name);
  if (mm && *mm) return *mm;
  auto m = NEWE(mdl);
  m->mdlnum = modelnum++;
  m->mmi = {2, 2, 0, 0, ""};
  m->loadname = NEWSTRING(name);
  mdllookup.access(m->loadname, &m);
  return m;
}

void mapmodel(const char *rad, const char *h, const char *zoff, const char *snap, const char *name) {
  auto m = loadmodel(name);
  m->mmi = { atoi(rad), atoi(h), atoi(zoff), atoi(snap), m->loadname };
  mapmodels.add(m);
}

void mapmodelreset(void) {
  auto &map = mdllookup;
  for (auto item : map) SAFE_DEL(item.second);
  mapmodels.setsize(0);
  modelnum = 0;
}

game::mapmodelinfo &getmminfo(int i) {
  return i<mapmodels.length() ? mapmodels[i]->mmi : *(game::mapmodelinfo*) NULL;
}

CMD(mapmodel, "sssss");
CMD(mapmodelreset, "");

void render(const char *name, int frame, int range,
            const vec3f &o, const vec3f &ypr,
            bool teammate, float scale, float speed, int snap, float basetime) {
  auto m = loadmodel(name);
  delayedload(m, scale, snap);
  ogl::bindtexture(GL_TEXTURE_2D, m->tex);
  m->render(frame, range, o, ypr, scale, speed, snap, basetime);
}
} /* namespace md2 */
} /* namespace q */

