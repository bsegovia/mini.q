/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - md2.cpp -> handles quake md2 models
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace md2 {

struct header {
  int magic;
  int version;
  int skinwidth, skinheight;
  int framesize;
  int numskins, numvertices, numtexcoords;
  int trianglenum, numglcommands, numframes;
  int offsetskins, offsettexcoords, offsettriangles;
  int offsetframes, offsetglcommands, offsetend;
};

struct vertex { u8 vertex[3], lightNormalIndex; };

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
    SAFE_DELETEA(glcommands);
    SAFE_DELETEA(frames);
    SAFE_DELETEA(builtframes);
    FREE(loadname);
  }
  bool load(char* filename);
  void render(int numframe, int range, const vec3f &o,
              const vec3f &ypr, float scale, float speed, int snap, float basetime);
  void scale(int frame, float scale, int sn);
  enum {channenum = 5}; // s,t,x,y,z
  int numglcommands;
  int *glcommands;
  u32 vbo, tex;
  int framesz;
  char *frames;
  bool *builtframes;
  game::mapmodelinfo mmi;
  char *loadname;
  int mdlnum;
  bool loaded;
};

bool mdl::load(char* filename) {
  FILE* file;
  header header;

  if ((file= fopen(filename, "rb"))==NULL) return false;

  fread(&header, sizeof(header), 1, file);
  sys::endianswap(&header, sizeof(int), sizeof(header)/sizeof(int));

  if (header.magic != 844121161 || header.version != 8) return false;
  frames = NEWAE(char,header.framesize*header.numframes);
  if (frames==NULL) return false;

  fseek(file, header.offsetframes, SEEK_SET);
  fread(frames, header.framesize*header.numframes, 1, file);
  loopi(header.numframes) sys::endianswap(frames+i*header.framesize,sizeof(float), 6);

  glcommands = NEWAE(int, header.numglcommands);
  if (glcommands==NULL) return false;
  fseek(file, header.offsetglcommands, SEEK_SET);
  fread(glcommands, header.numglcommands*sizeof(int), 1, file);
  sys::endianswap(glcommands, sizeof(int), header.numglcommands);

  numglcommands = header.numglcommands;

  fclose(file);

  builtframes = NEWAE(bool, header.numframes);
  loopj(header.numframes) builtframes[j] = false;

  // allocate one vbo for all key frames together
  for (int *command=glcommands, i=0; (*command)!=0; ++i) {
    const int n = abs(*command++);
    framesz += 3*(n-2)*sizeof(float[channenum]);
    command += 3*n; // +1 for index, +2 for u,v
  }
  ogl::genbuffers(1, &vbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  OGL(BufferData, GL_ARRAY_BUFFER, header.numframes*framesz, NULL, GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  return true;
}

static float snap(int sn, float f) {
  return sn ? (float)(((int)(f+sn*0.5f))&(~(sn-1))) : f;
}

void mdl::scale(int frame, float scale, int sn) {
  builtframes[frame] = true;
  const auto cf = (const md2::frame*) ((char*)frames+framesz*frame);
  const float sc = 16.0f/scale;

  vector<array<float,channenum>> tris;
  for (int *command = glcommands, i=0; (*command)!=0; ++i) {
    const int moden = *command++;
    const int n = abs(moden);
    vector<array<float,channenum>> trisv;
    loopi(n) {
      const float s = *((const float*)command++);
      const float t = *((const float*)command++);
      const int vn = *command++;
      const u8 *cv = (u8 *)&cf->vertices[vn].vertex;
      const vec3f v(+(snap(sn, cv[0]*cf->scale[0])+cf->translate[0])/sc,
                  -(snap(sn, cv[1]*cf->scale[1])+cf->translate[1])/sc,
                  +(snap(sn, cv[2]*cf->scale[2])+cf->translate[2])/sc);
      trisv.add(array<float,channenum>(s,t,v.x,v.z,v.y));
    }
    loopi(n-2) { // just stolen from sauer. TODO use an index buffer
      if (moden <= 0) { // fan
        tris.add(trisv[0]);
        tris.add(trisv[i+1]);
        tris.add(trisv[i+2]);
      } else // strip
        loopk(3) tris.add(trisv[i&1 && k ? i+(1-(k-1))+1 : i+k]);
    }
  }
  OGL(BufferSubData, GL_ARRAY_BUFFER, framesz*frame, framesz, &tris[0][0]);
}

void mdl::render(int frame, int range, const vec3f &o,
                const vec3f &ypr, float sc, float speed, int snap, float basetime) {
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  loopi(range) if (!builtframes[frame+i]) scale(frame+i, sc, snap);

  ogl::pushmatrix();
  ogl::translate(o);
  ogl::rotate(ypr.x+180.f, vec3f(0.f, -1.f, 0.f));
  ogl::rotate(ypr.y, vec3f(0.f, 0.f, 1.f));

  const int n = framesz / sizeof(float[channenum]);
  const float time = game::lastmillis-basetime;
  intptr_t fr1 = (intptr_t)(time/speed);
  const float frac = (time-fr1*speed)/speed;
  fr1 = fr1%range+frame;
  intptr_t fr2 = fr1+1u;
  if (fr2>=frame+range) fr2 = frame;
  auto pos0 = (const float*)(fr1*framesz);
  auto pos1 = (const float*)(fr2*framesz);
  // ogl::rendermd2(pos0, pos1, frac, n);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::popmatrix();
}

static vector<mdl*> mapmodels;
static hashtable<mdl*> mdllookup;
static hashtable<u32> texlookup;
static const int FIRSTMDL = 20;

static void delayedload(mdl *m) {
  if (!m->loaded) {
    sprintf_sd(mdlpath)("packages/models/%s/tris.md2", m->loadname);
    if (!m->load(sys::path(mdlpath))) sys::fatal("loadmodel: ", mdlpath);
    sprintf_sd(texpath)("packages/models/%s/skin.jpg", m->loadname);
    m->tex = ogl::installtex(texpath);
    m->loaded = true;
    texlookup.access(texpath, &m->tex);
  }
}

static int modelnum = 0;

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
  for (auto item : map) SAFE_DELETE(item.second);
  mapmodels.setsize(0);
  modelnum = 0;
}

game::mapmodelinfo &getmminfo(int i) {
  return i<mapmodels.length() ? mapmodels[i]->mmi : *(game::mapmodelinfo*) NULL;
}

CMD(mapmodel, "sssss");
CMD(mapmodelreset, "");

void render(const char *name, int frame, int range,
            float rad, const vec3f &o, const vec3f &ypr,
            bool teammate, float scale, float speed, int snap, float basetime) {
  auto m = loadmodel(name);
  delayedload(m);
  ogl::bindtexture(GL_TEXTURE_2D, m->tex);
  m->render(frame, range, o, ypr, scale, speed, snap, basetime);
}
} /* namespace md2 */
} /* namespace q */

