/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ogl.cpp -> defines opengl routines
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "sys.hpp"
#include "shaders.hpp"
#include "script.hpp"
#include "game.hpp"
#include "math.hpp"
#include "text.hpp"
#include "ogl.hpp"

#include "GL/glext.h"
#include <SDL/SDL_image.h>

namespace q {
namespace ogl {

/*-------------------------------------------------------------------------
 - OGL extensions / versions...
 -------------------------------------------------------------------------*/
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#define OGLPROC(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */
static void *getfunction(const char *name) {
  void *ptr = SDL_GL_GetProcAddress(name);
  if (ptr == NULL) sys::fatal("OpenGL 3.0 is required");
  return ptr;
}

static u32 glversion = 100, glslversion = 100;
static bool mesa = false, intel = false, nvidia = false, amd = false;
static bool hasTQ = false;
static u32 hwtexunits = 0, hwvtexunits = 0, hwtexsize = 0, hwcubetexsize = 0;

static PFNGLGETQUERYOBJECTI64VEXTPROC GetQueryObjecti64v = NULL;
static PFNGLGETQUERYOBJECTUI64VEXTPROC GetQueryObjectui64v = NULL;

struct glext {
  ~glext() { for (auto item : glexts) FREE(item.second); }
  void parse() {
    const char *exts = (const char *) ogl::GetString(GL_EXTENSIONS);
    for(;;) {
      while (*exts == ' ') exts++;
      if (!*exts) break;
      const char *ext = exts;
      while (*exts && *exts != ' ') exts++;
      if (exts > ext) {
        const auto str = NEWSTRING(ext, size_t(exts-ext));
        glexts.access(str,&str);
      }
    }
  }
  bool has(const char *ext) {return glexts.access(ext)!=NULL;}
  hashtable<char*> glexts;
};

static void startgl() {
// load OGL 1.1 first
#if !defined(__WEBGL__)
#define OGLPROC(FIELD,NAME,PROTOTYPE)
#if defined(__WIN32__)
  #define OGLPROC110(FIELD,NAME,PROTOTYPE) FIELD = (PROTOTYPE) NAME;
#else
  #define OGLPROC110(FIELD,NAME,PROTO) FIELD = (PROTO) getfunction(#NAME);
#endif /* __WIN32__ */
#include "ogl.hxx"
#undef OGLPROC110
#undef OGLPROC
#endif /* __WEBGL__ */

  glext ext;
  ext.parse();
  const auto vendor = (const char *) ogl::GetString(GL_VENDOR);
  const auto renderer = (const char *) ogl::GetString(GL_RENDERER);
  const auto version = (const char *) ogl::GetString(GL_VERSION);
  con::out("ogl: renderer: %s (%s)", renderer, vendor);
  con::out("ogl: driver: %s", version);

  if (strstr(renderer, "Mesa") || strstr(version, "Mesa")) {
    mesa = true;
    if (strstr(renderer, "Intel")) intel = true;
  } else if (strstr(vendor, "NVIDIA"))
    nvidia = true;
  else if (strstr(vendor, "ATI") || strstr(vendor, "Advanced Micro Devices"))
    amd = true;
  else if (strstr(vendor, "Intel"))
    intel = true;

  u32 glmajorversion, glminorversion;
  if (sscanf(version, " %u.%u", &glmajorversion, &glminorversion) != 2)
    glversion = 100;
  else
    glversion = glmajorversion*100 + glminorversion*10;

  if (glversion < 300) sys::fatal("OpenGL 2.1 or greater is required");

  const auto glslstr = (const char*) ogl::GetString(GL_SHADING_LANGUAGE_VERSION);
  con::out("ogl: glsl: %s", glslstr ? glslstr : "unknown");

  u32 glslmajorversion, glslminorversion;
  if (glslstr && sscanf(glslstr, " %u.%u", &glslmajorversion, &glslminorversion) == 2)
    glslversion = glslmajorversion*100 + glslminorversion;
  if (glslversion < 130)
    sys::fatal("OpenGL: GLSL 1.30 or greater is required!");

// load OGL 3.0 now
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE)
#define OGLPROC(FIELD,NAME,PROTO) FIELD = (PROTO) getfunction(#NAME);
#include "ogl.hxx"
#undef OGLPROC110
#undef OGLPROC
#endif /* __WEBGL__ */

  GLint texsize = 0, texunits = 0, vtexunits = 0, cubetexsize = 0, drawbufs = 0;
  ogl::GetIntegerv(GL_MAX_TEXTURE_SIZE, &texsize);
  hwtexsize = texsize;
  if (hwtexsize < 4096)
    sys::fatal("OpenGL: large texture support is required!");
  ogl::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texunits);
  hwtexunits = texunits;
  if (hwtexunits < 16)
    sys::fatal("OpenGL: hardware does not support at least 16 texture units.");
  ogl::GetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &vtexunits);
  hwvtexunits = vtexunits;
  if (hwvtexunits < 4)
    sys::fatal("OpenGL: hardware does not support at least 4 vertex texture units.");
  ogl::GetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &cubetexsize);
  hwcubetexsize = cubetexsize;
  ogl::GetIntegerv(GL_MAX_DRAW_BUFFERS, &drawbufs);
  if (drawbufs < 4) sys::fatal("OpenGL: hardware does not support at least 4 draw buffers.");

  if (!ext.has("GL_ARB_texture_rectangle"))
    sys::fatal("OpenGL: GL_ARB_texture_rectangle extension is required");
  if (ext.has("GL_EXT_timer_query") || ext.has("GL_ARB_timer_query")) {
    if(ext.has("GL_EXT_timer_query")) {
      GetQueryObjecti64v  = (PFNGLGETQUERYOBJECTI64VEXTPROC)  getfunction("glGetQueryObjecti64vEXT");
      GetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VEXTPROC) getfunction("glGetQueryObjectui64vEXT");
    } else {
      GetQueryObjecti64v  = (PFNGLGETQUERYOBJECTI64VEXTPROC)  getfunction("glGetQueryObjecti64v");
      GetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VEXTPROC) getfunction("glGetQueryObjectui64v");
    }
    hasTQ = true;
  }
}

/*-------------------------------------------------------------------------
 - very simple state tracking
 -------------------------------------------------------------------------*/
union {
  struct {
    u32 shader:1; // will force to reload everything
    u32 mvp:1;
  } flags;
  u32 any;
} dirty;
static u32 bindedvbo[BUFFER_NUM];
static u32 enabledattribarray[ATTRIB_NUM];
static struct shadertype *bindedshader = NULL;

static const u32 TEX_NUM = 8;
static u32 bindedtexture[TEX_NUM];

void enableattribarray(u32 target) {
  if (!enabledattribarray[target]) {
    enabledattribarray[target] = 1;
    OGL(EnableVertexAttribArray, target);
  }
}

void disableattribarray(u32 target) {
  if (enabledattribarray[target]) {
    enabledattribarray[target] = 0;
    OGL(DisableVertexAttribArray, target);
  }
}

/*--------------------------------------------------------------------------
 - simple resource management
 -------------------------------------------------------------------------*/
static s32 texturenum=0, buffernum=0, programnum=0, framebuffernum=0;

void gentextures(s32 n, u32 *id) {
  texturenum += n;
  OGL(GenTextures, n, id);
}
void deletetextures(s32 n, u32 *id) {
  texturenum -= n;
  assert(texturenum >= 0);
  OGL(DeleteTextures, n, id);
}
void genbuffers(s32 n, u32 *id) {
  buffernum += n;
  OGL(GenBuffers, n, id);
}
void deletebuffers(s32 n, u32 *id) {
  buffernum -= n;
  assert(buffernum >= 0);
  OGL(DeleteBuffers, n, id);
}
void genframebuffers(s32 n, u32 *id) {
  framebuffernum += n;
  OGL(GenFramebuffers, n, id);
}
void deleteframebuffers(s32 n, u32 *id) {
  framebuffernum -= n;
  assert(framebuffernum >= 0);
  OGL(DeleteFramebuffers, n, id);
}

static u32 createprogram() {
  programnum++;
#if defined(__WEBGL__)
  return glCreateProgram();
#else
  return CreateProgram();
#endif // __WEBGL__
}
static void deleteprogram(u32 id) {
  programnum--;
  if (programnum < 0) sys::fatal("program already freed");
  OGL(DeleteProgram, id);
}

/*-------------------------------------------------------------------------
 - matrix handling. _very_ inspired by opengl
 -------------------------------------------------------------------------*/
enum {MATRIX_STACK = 4};
static mat4x4f vp[MATRIX_MODE] = {one, one};
static mat4x4f vpstack[MATRIX_STACK][MATRIX_MODE];
static int vpdepth = 0;
static int vpmode = MODELVIEW;
static mat4x4f viewproj(one);

void matrixmode(int mode) { vpmode = mode; }
const mat4x4f &matrix(int mode) { return vp[mode]; }
void loadmatrix(const mat4x4f &m) {
  dirty.flags.mvp=1;
  vp[vpmode] = m;
}
void identity() {
  dirty.flags.mvp=1;
  vp[vpmode] = mat4x4f(one);
}
void translate(const vec3f &v) {
  dirty.flags.mvp=1;
  vp[vpmode] = vp[vpmode]*mat4x4f::translate(v);
}
void mulmatrix(const mat4x4f &m) {
  dirty.flags.mvp=1;
  vp[vpmode] = m*vp[vpmode];
}
void rotate(float angle, const vec3f &axis) {
  dirty.flags.mvp=1;
  vp[vpmode] = vp[vpmode]*mat4x4f::rotate(angle,axis);
}
void setperspective(float fovy, float aspect, float znear, float zfar) {
  dirty.flags.mvp=1;
  vp[vpmode] = /*vp[vpmode]**/q::perspective(fovy,aspect,znear,zfar);
}
void setortho(float left, float right, float bottom, float top, float znear, float zfar) {
  dirty.flags.mvp=1;
  vp[vpmode] = /*vp[vpmode]**/q::ortho(left,right,bottom,top,znear,zfar);
}
void scale(const vec3f &s) {
  dirty.flags.mvp=1;
  vp[vpmode] = scale(vp[vpmode],s);
}
void pushmatrix() {
  assert(vpdepth+1<MATRIX_STACK);
  vpstack[vpdepth++][vpmode] = vp[vpmode];
}
void popmatrix() {
  assert(vpdepth>0);
  dirty.flags.mvp=1;
  vp[vpmode] = vpstack[--vpdepth][vpmode];
}

/*--------------------------------------------------------------------------
 - very simple texture support
 -------------------------------------------------------------------------*/
static int glmaxtexsize = 256;
void bindtexture(u32 target, u32 id, u32 texslot) {
  if (bindedtexture[texslot] == id) return;
  bindedtexture[texslot] = id;
  OGL(ActiveTexture, GL_TEXTURE0 + texslot);
  OGL(BindTexture, target, id);
}

u32 maketex(const char *fmt, ...) {
  va_list args;
  u32 id, target = GL_TEXTURE_2D, internalfmt = GL_RGBA;
  u32 datafmt = GL_RGBA, type = GL_UNSIGNED_BYTE, filter = GL_NEAREST;
  u32 wrap = GL_TEXTURE_WRAP_S, wrapmode = GL_CLAMP_TO_EDGE;
  u32 dim[3] = {0,0,0}, dimnum = 0;
  void *data = NULL;
  char ch = '\0';

#define PEEK \
  ch = *fmt; ++fmt; ch = ch=='%' ? char(va_arg(args,int)) : ch; \
  switch (ch)

  gentextures(1, &id);
  va_start(args, fmt);
  while (*fmt) {
    PEEK {
      case 'B': // build the texture
        PEEK {
          case '1': target = GL_TEXTURE_1D; dimnum = 1; break;
          case '2': target = GL_TEXTURE_2D; dimnum = 2; break;
          case '3': target = GL_TEXTURE_3D; dimnum = 3; break;
          case 'r': target = GL_TEXTURE_RECTANGLE; dimnum = 2; break;
        }
        ogl::bindtexture(target, id, 0);
        OGL(PixelStorei, GL_UNPACK_ALIGNMENT, 1);
        data = va_arg(args, void*);
        loopi(dimnum) dim[i] = va_arg(args, u32);
        if (dimnum == 1)
          OGL(TexImage1D, target, 0, internalfmt, dim[0], 0, datafmt, type, data);
        else if (dimnum == 2)
          OGL(TexImage2D, target, 0, internalfmt, dim[0], dim[1], 0, datafmt, type, data);
        else if (dimnum == 3)
          OGL(TexImage3D, target, 0, internalfmt, dim[0], dim[1], dim[2], 0, datafmt, type, data);
      break;
      case 'G': OGL(GenerateMipmap, GL_TEXTURE_2D); break;
      case 'T': PEEK { // type
        case 'b': type = GL_BYTE; break;
        case 'B': type = GL_UNSIGNED_BYTE; break;
        case 's': type = GL_SHORT; break;
        case 'S': type = GL_UNSIGNED_SHORT; break;
        case 'i': type = GL_INT; break;
        case 'I': type = GL_UNSIGNED_INT; break;
        case 'f': type = GL_FLOAT; break;
      }
      break;
      case 'D': PEEK { // data format
        case '3': datafmt = GL_RGB; break;
        case '4': datafmt = GL_RGBA; break;
        case 'r': datafmt = GL_RED; break;
        case 'a': datafmt = GL_ALPHA; break;
        case 'd': datafmt = GL_DEPTH_COMPONENT; break;
      }
      break;
      case 'I': PEEK { // internal data format
        case '3': internalfmt = GL_RGB; break;
        case '4': internalfmt = GL_RGBA; break;
        case 'r': internalfmt = GL_RED; break;
        case 'a': internalfmt = GL_ALPHA; break;
        case 'd': internalfmt = GL_DEPTH_COMPONENT32; break;
      }
      break;
      case 'm': // minfilter
        PEEK {
          case 'n': filter = GL_NEAREST; break;
          case 'l': filter = GL_LINEAR; break;
          case 'm': filter = GL_NEAREST_MIPMAP_LINEAR; break;
          case 'N': filter = GL_NEAREST_MIPMAP_NEAREST; break;
          case 'L': filter = GL_LINEAR_MIPMAP_NEAREST; break;
          case 'M': filter = GL_LINEAR_MIPMAP_LINEAR; break;
        }
        OGL(TexParameteri, target, GL_TEXTURE_MIN_FILTER, filter);
      break;
      case 'M': // magfilter
        PEEK {
          case 'n': filter = GL_NEAREST; break;
          case 'l': filter = GL_LINEAR; break;
        }
        OGL(TexParameteri, target, GL_TEXTURE_MAG_FILTER, filter);
      break;
      case 'W': // wrap mode
        PEEK {
          case 's': wrap = GL_TEXTURE_WRAP_S; break;
          case 't': wrap = GL_TEXTURE_WRAP_T; break;
          case 'r': wrap = GL_TEXTURE_WRAP_R; break;
        }
        PEEK {
          case 'e': wrapmode = GL_CLAMP_TO_EDGE; break;
          case 'b': wrapmode = GL_CLAMP_TO_BORDER; break;
          case 'r': wrapmode = GL_REPEAT; break;
          case 'm': wrapmode = GL_MIRRORED_REPEAT; break;
        }
        OGL(TexParameteri, target, wrap, wrapmode);
      break;
    }
  }
#undef PEEK
  va_end(args);
  return id;
}

u32 installtex(const char *texname, bool clamp) {
  auto s = IMG_Load(texname);
  if (!s) {
    con::out("couldn't load texture %s", texname);
    return 0;
  }
#if !defined(__WEBGL__)
  else if (s->format->BitsPerPixel!=24) {
    con::out("texture must be 24bpp: %s (got %i bpp)", texname, s->format->BitsPerPixel);
    return 0;
  }
#endif // __WEBGL__

  loopi(int(TEX_NUM)) bindedtexture[i] = 0;
  con::out("loading %s (%ix%i)", texname, s->w, s->h);
  if (s->w>glmaxtexsize || s->h>glmaxtexsize)
    sys::fatal("texture dimensions are too large");
  const auto ispowerof2 = ispoweroftwo(s->w) && ispoweroftwo(s->h);
  const auto minf = ispowerof2 ? 'l' : 'M';
  const auto mm = ispowerof2 ? 'G' : ' ';
  const auto fmt = s->format->BitsPerPixel == 24 ? '3' : '4';
  const auto wrap = clamp ? 'e' : 'r';
  auto id = maketex("TB I% D% B2 % Ws% Wt% Ml m%",fmt,fmt,s->pixels,s->w,s->h,mm,wrap,wrap,minf);
  SDL_FreeSurface(s);
  return id;
}

/*--------------------------------------------------------------------------
 - immediate mode and buffer support
 -------------------------------------------------------------------------*/
static const u32 glbufferbinding[BUFFER_NUM] = {
  GL_ARRAY_BUFFER,
  GL_ELEMENT_ARRAY_BUFFER
};
void bindbuffer(u32 target, u32 buffer) {
  if (bindedvbo[target] != buffer) {
    OGL(BindBuffer, glbufferbinding[target], buffer);
    bindedvbo[target] = buffer;
  }
}

// attributes in the vertex buffer
static struct {int n, type, offset;} immattribs[ATTRIB_NUM];
static int immvertexsz = 0;

// we use two big circular buffers to handle immediate mode
static int immbuffersize = 16*1024*1024;
static int bigvbooffset=0, bigibooffset=0;
static int drawibooffset=0, drawvbooffset=0;
static u32 bigvbo=0u, bigibo=0u;

static void initbuffer(u32 &bo, int target, int size) {
  if (bo == 0u) genbuffers(1, &bo);
  bindbuffer(target, bo);
  OGL(BufferData, glbufferbinding[target], size, NULL, GL_DYNAMIC_DRAW);
  bindbuffer(target, 0);
}

static void imminit() {
  initbuffer(bigvbo, ARRAY_BUFFER, immbuffersize);
  initbuffer(bigibo, ELEMENT_ARRAY_BUFFER, immbuffersize);
  memset(immattribs, 0, sizeof(immattribs));
}
static void immdestroy() {
  ogl::deletebuffers(1, &bigibo);
  ogl::deletebuffers(1, &bigvbo);
}

static void immattrib(int attrib, int n, int type, int offset) {
  immattribs[attrib].n = n;
  immattribs[attrib].type = type;
  immattribs[attrib].offset = offset;
}

static void immvertexsize(int sz) { immvertexsz = sz; }

static void immsetallattribs() {
  loopi(ATTRIB_NUM) {
    if (!enabledattribarray[i]) continue;
    const void *fake = (const void *) intptr_t(drawvbooffset+immattribs[i].offset);
    OGL(VertexAttribPointer, i, immattribs[i].n, immattribs[i].type, 0, immvertexsz, fake);
  }
}

static bool immsetdata(int target, int sz, const void *data) {
  if (sz >= immbuffersize) {
    con::out("too many immediate items to render");
    return false;
  }
  u32 &bo = target==ARRAY_BUFFER ? bigvbo : bigibo;
  int &offset = target==ARRAY_BUFFER ? bigvbooffset : bigibooffset;
  int &drawoffset = target==ARRAY_BUFFER ? drawvbooffset : drawibooffset;
  if (offset+sz > immbuffersize) {
    OGL(Flush);
    bindbuffer(target, 0);
    offset = 0u;
    bindbuffer(target, bo);
  }
  bindbuffer(target, bo);
  OGL(BufferSubData, glbufferbinding[target], offset, sz, data);
  drawoffset = offset;
  offset += sz;
  return true;
}

static bool immvertices(int sz, const void *vertices) {
  return immsetdata(ARRAY_BUFFER, sz, vertices);
}

void immdrawarrays(int mode, int first, int count) {
  if (bindedshader->fixedfunction) fixedflush();
  drawarrays(mode,first,count);
}

void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices)
{
  int indexsz = count;
  int maxindex = 0;
  switch (type) {
    case GL_UNSIGNED_INT:
      indexsz*=sizeof(u32);
      loopi(count) maxindex = max(maxindex,int(((u32*)indices)[i]));
    break;
    case GL_UNSIGNED_SHORT:
      indexsz*=sizeof(u16);
      loopi(count) maxindex = max(maxindex,int(((u16*)indices)[i]));
    break;
    case GL_UNSIGNED_BYTE:
      indexsz*=sizeof(u8);
      loopi(count) maxindex = max(maxindex,int(((u8*)indices)[i]));
    break;
  };
  if (!immsetdata(ELEMENT_ARRAY_BUFFER, indexsz, indices)) return;
  if (!immvertices((maxindex+1)*immvertexsz, vertices)) return;
  immsetallattribs();
  if (bindedshader->fixedfunction) fixedflush();
  const void *fake = (const void *) intptr_t(drawibooffset);
  drawelements(mode, count, type, fake);
}

// return {size, mode, type}
static vec3i parseformat(const char *fmt) {
  loopi(ATTRIB_NUM) disableattribarray(i);
  int mode = GL_TRIANGLE_STRIP, type = GL_UNSIGNED_INT;
  int offset = 0;
  while (*fmt) {
    switch (*fmt) {
      case 'T': mode = GL_TRIANGLES; break;
      case 'F': mode = GL_TRIANGLE_FAN; break;
      case 'S': mode = GL_TRIANGLE_STRIP; break;
      case 'i': type = GL_UNSIGNED_INT; break;
      case 's': type = GL_UNSIGNED_SHORT; break;
      case 'b': type = GL_UNSIGNED_BYTE; break;
#define ATTRIB(CHAR, GL)\
  case CHAR: ++fmt;\
    enableattribarray(GL);\
    immattrib(GL, int(*fmt-'0'), GL_FLOAT, offset);\
    offset += int(*fmt-'0')*int(sizeof(float));\
  break;
  ATTRIB('p', ATTRIB_POS0);
  ATTRIB('t', ATTRIB_TEX0);
  ATTRIB('c', ATTRIB_COL);
#undef ATTRIB
    }
    ++fmt;
  }
  return vec3i(offset, mode, type);
}

void immdrawelememts(const char *fmt, int count, const void *indices, const void *vertices)
{
  const auto parsed = parseformat(fmt);
  immvertexsize(parsed.x);
  immdrawelements(parsed.y, count, parsed.z, indices, vertices);
}

void immdraw(const char *fmt, int count, const void *data) {
  const auto parsed = parseformat(fmt);
  if (!immvertices(count*parsed.x, data)) return;
  immvertexsize(parsed.x);
  immsetallattribs();
  immdrawarrays(parsed.y, 0, count);
}

/*-------------------------------------------------------------------------
 - GPU timer (taken from Tesseract)
 -------------------------------------------------------------------------*/
struct timer {
  enum { MAXQUERY = 4 };
  const char *name;
  bool gpu;
  u32 query[MAXQUERY];
  int waiting;
  float starttime;
  float result, print;
};
static vector<timer> timers;
static vector<int> timerorder;
static int timercycle = 0;

extern int gputimers;
static int deferquery=0;

static timer *findtimer(const char *name, bool gpu) {
  loopv(timers) if (!strcmp(timers[i].name, name) && timers[i].gpu == gpu) {
    timerorder.removeobj(i);
    timerorder.add(i);
    return &timers[i];
  }
  timerorder.add(timers.length());
  auto &t = timers.add();
  t.name = name;
  t.gpu = gpu;
  memset(t.query, 0, sizeof(t.query));
  if (gpu) OGL(GenQueries, timer::MAXQUERY, t.query);
  t.waiting = 0;
  t.starttime = 0.f;
  t.result = -1;
  t.print = -1;
  return &t;
}

timer *begintimer(const char *name, bool gpu) {
  if (!gputimers || (gpu && (!hasTQ || deferquery)))
    return NULL;
  const auto t = findtimer(name, gpu);
  if (t->gpu) {
    deferquery++;
    OGL(BeginQuery, GL_TIME_ELAPSED_EXT, t->query[timercycle]);
    t->waiting |= 1<<timercycle;
  } else
    t->starttime = sys::millis();
  return t;
}

void endtimer(timer *t) {
  if (!t) return;
  if (t->gpu) {
    OGL(EndQuery, GL_TIME_ELAPSED_EXT);
    deferquery--;
  } else
    t->result = max(sys::millis() - t->starttime, 0.0f);
}

static void synctimers() {
  timercycle = (timercycle + 1) % timer::MAXQUERY;
  loopv(timers) {
    auto &t = timers[i];
    if (t.waiting&(1<<timercycle)) {
      GLint available = 0;
      while (!available)
        OGL(GetQueryObjectiv, t.query[timercycle], GL_QUERY_RESULT_AVAILABLE, &available);
      GLuint64EXT result = 0;
      OGL(GetQueryObjectui64v, t.query[timercycle], GL_QUERY_RESULT, &result);
      t.result = max(float(result) * 1e-6f, 0.0f);
      t.waiting &= ~(1<<timercycle);
    } else
      t.result = -1;
  }
}

static void cleanuptimers() {
  loopv(timers) {
    timer &t = timers[i];
    if (t.gpu) OGL(DeleteQueries, timer::MAXQUERY, t.query);
  }
  timers.destroy();
  timerorder.destroy();
}

IVARF(gputimers, 0, 0, 1, cleanuptimers());
IVAR(frametimer, 0, 0, 1);
static float framemillis = 0.f, totalmillis = 0.f;

void printtimers(float conw, float conh) {
  if (!frametimer && !gputimers) return;

  static float lastprint = 0.f;
  int offset = 0;
  const auto dim = text::fontdim();
  if (frametimer) {
    static float printmillis = 0.f;
    if (totalmillis - lastprint >= 200.f)
      printmillis = framemillis;
    const vec2f tp(conw-20.f*dim.y, conh-dim.y*3.f/2.f-float(offset)*9.f*dim.y/8.f);
    text::drawf("frame time %5.2 ms", tp, printmillis);
    offset++;
  }
  if (gputimers) loopv(timerorder) {
    auto &t = timers[timerorder[i]];
    if (t.print < 0.f ? t.result >= 0.f : totalmillis - lastprint >= 200.f)
      t.print = t.result;
    if (t.print < 0 || (t.gpu && !(t.waiting&(1<<timercycle))))
      continue;
    const vec2f tp(conw-20.f*dim.y, conh-dim.y*3.f/2.f-offset*9.f*dim.y/8.f);
    text::drawf("%s%s %5.2f ms", tp, t.name, t.gpu ? "" : " (cpu)", t.print);
    ++offset;
  }
  if (totalmillis - lastprint >= 200.f)
    lastprint = totalmillis;
}

void beginframe() {
  synctimers();
  totalmillis = sys::millis();
}

void endframe() {
  if (frametimer) {
    OGL(Finish);
    framemillis = sys::millis() - totalmillis;
  }
}

/*--------------------------------------------------------------------------
 - shader management
 -------------------------------------------------------------------------*/
static bool checkshader(const char *source, u32 shadernumame) {
  GLint result = GL_FALSE;
  int infologlength;

  if (!shadernumame) return false;
  OGL(GetShaderiv, shadernumame, GL_COMPILE_STATUS, &result);
  OGL(GetShaderiv, shadernumame, GL_INFO_LOG_LENGTH, &infologlength);
  if (infologlength > 1) {
    char *buffer = (char*) MALLOC(infologlength+1);
    buffer[infologlength] = 0;
    OGL(GetShaderInfoLog, shadernumame, infologlength, NULL, buffer);
    con::out("%s", buffer);
    printf("in\n%s\n", source);
    FREE(buffer);
  }
  return result == GL_TRUE;
}

static const char header[] = {
#if defined(__WEBGL__)
  "precision highp float;\n"
  "#define IF_WEBGL(X) X\n"
  "#define IF_NOT_WEBGL(X)\n"
  "#define SWITCH_WEBGL(X,Y) X\n"
  "#define VS_IN attribute\n"
  "#define VS_OUT varying\n"
  "#define PS_IN varying\n"
#else
  "#version 130\n"
  "#extension GL_ARB_texture_rectangle : enable\n"
  "#define IF_WEBGL(X)\n"
  "#define IF_NOT_WEBGL(X) X\n"
  "#define SWITCH_WEBGL(X,Y) Y\n"
  "#define VS_IN in\n"
  "#define VS_OUT out\n"
  "#define PS_IN in\n"
#endif // __WEBGL__
};

static u32 loadshader(GLenum type, const char *source, const shaderrules &rules) {
  u32 name;
  OGLR(name, CreateShader, type);
  vector<const char*> sources;
  sources.add(header);
  loopv(rules) sources.add(rules[i]);
  sources.add(source);
  OGL(ShaderSource, name, rules.length()+2, &sources[0], NULL);
  OGL(CompileShader, name);
  if (!checkshader(source, name)) return 0;
  return name;
}

static void linkshader(shadertype &shader) {
  OGL(LinkProgram, shader.program);
  OGL(ValidateProgram, shader.program);
}

static u32 loadprogram(const char *vertstr, const char *fragstr,
                       const shaderrules &vertrules,
                       const shaderrules &fragrules)
{
  u32 program = 0;
  const u32 vert = loadshader(GL_VERTEX_SHADER, vertstr, vertrules);
  const u32 frag = loadshader(GL_FRAGMENT_SHADER, fragstr, fragrules);
  if (vert == 0 || frag == 0) return 0;
  program = createprogram();
  OGL(AttachShader, program, vert);
  OGL(AttachShader, program, frag);
  OGL(DeleteShader, vert);
  OGL(DeleteShader, frag);
  return program;
}
#undef OGL_PROGRAM_HEADER

u32 shaderbuilder::compile(const char *vert, const char *frag) {
  u32 program;
  vector<char*> fragrules, vertrules;
  setrules(fragrules, vertrules);
  if ((program = loadprogram(vert, frag, fragrules, vertrules))==0) return 0;
  loopv(vertrules) FREE(vertrules[i]);
  loopv(fragrules) FREE(fragrules[i]);
  return program;
}

bool shaderbuilder::buildprogram(shadertype &s, const char *vert, const char *frag) {
  const auto program = compile(vert, frag);
  if (program == 0) return false;
  if (s.program) deleteprogram(s.program);
  s.program = program;
  setinout(s);
  linkshader(s);
  OGL(UseProgram, s.program);
  setuniform(s);
  OGL(UseProgram, 0);
  dirty.any = ~0x0;
  return true;
}

static char *loadshaderfile(const char *path) {
  auto s = sys::loadfile(path);
  if (s == NULL) con::out("unable to load shader %s", path);
  return s;
}

bool shaderbuilder::buildprogramfromfile(shadertype &s) {
  auto fixed_vp = loadshaderfile(vppath);
  auto fixed_fp = loadshaderfile(fppath);
  if (fixed_fp == NULL || fixed_vp == NULL) return false;
  auto ret = buildprogram(s, fixed_vp, fixed_fp);
  FREE(fixed_fp);
  FREE(fixed_vp);
  return ret;
}

// use to rebuild shaders on-demand later
static vector<pair<shaderbuilder*, shadertype*>> allshaders;

bool shaderbuilder::build(shadertype &s, int fromfile, bool save) {
  if (save) allshaders.add(makepair(this, &s));
  if (fromfile)
    return buildprogramfromfile(s);
  else
    return buildprogram(s, vp, fp);
}

void bindshader(shadertype &shader) {
  if (bindedshader != &shader) {
    bindedshader = &shader;
    dirty.any = ~0x0;
    OGL(UseProgram, bindedshader->program);
  }
}

void destroyshader(shadertype &s) {
  deleteprogram(s.program);
  s.program = 0;
}

IVAR(shaderfromfile, 0, 1, 1);
bool loadfromfile() { return shaderfromfile; }

void shadererror(bool fatalerr, const char *msg) {
  if (fatalerr)
    sys::fatal("unable to build fixed shaders %s", msg);
  else
    con::out("unable to build fixed shaders %s", msg);
}

#if !defined(RELEASE)
static void reloadshaders() {
  loopv(allshaders) {
    auto &s = allshaders[i];
    s.first->build(*s.second, shaderfromfile, false);
  }
}
CMD(reloadshaders, "");
#endif

/*--------------------------------------------------------------------------
 - fixed pipeline
 -------------------------------------------------------------------------*/
static fixedshadertype shaders[fixedshadernum];

// flush all the states required for the draw call
void fixedflush() {
  if (dirty.any == 0) return; // fast path
  if (dirty.flags.mvp) {
    const auto s = static_cast<fixedshadertype*>(bindedshader);
    viewproj = vp[PROJECTION]*vp[MODELVIEW];
    OGL(UniformMatrix4fv, s->u_mvp, 1, GL_FALSE, &viewproj.vx.x);
    dirty.flags.mvp = 0;
  }
}

void bindfixedshader(u32 flags) { bindshader(shaders[flags]); }
void bindfixedshader(u32 flags, float delta) {
  bindfixedshader(flags);
  OGL(Uniform1f, static_cast<fixedshadertype*>(bindedshader)->u_delta, delta);
}

fixedshaderbuilder::fixedshaderbuilder(const char *vppath, const char *fppath,
                                       const char *vp, const char *fp,
                                       u32 rules) :
  shaderbuilder(vppath, fppath, vp, fp), rules(rules) {}

void fixedshaderbuilder::setrules(shaderrules &vertrules, shaderrules &fragrules) {
  sprintf_sd(str)("#define USE_COL %d\n"
                 "#define USE_KEYFRAME %d\n"
                 "#define USE_DIFFUSETEX %d\n",
                 rules&FIXED_COLOR,
                 rules&FIXED_KEYFRAME,
                 rules&FIXED_DIFFUSETEX);
  vertrules.add(NEWSTRING(str));
  fragrules.add(NEWSTRING(str));
}

void fixedshaderbuilder::setuniform(shadertype &s) {
  auto &shader = static_cast<fixedshadertype&>(s);
  OGLR(shader.u_mvp, GetUniformLocation, shader.program, "u_mvp");
  if (rules&FIXED_KEYFRAME)
    OGLR(shader.u_delta, GetUniformLocation, shader.program, "u_delta");
  else
    shader.u_delta = 0;
  if (rules&FIXED_DIFFUSETEX) {
    OGLR(shader.u_diffuse, GetUniformLocation, shader.program, "u_diffuse");
    OGL(Uniform1i, shader.u_diffuse, 0);
  }
}

void fixedshaderbuilder::setinout(shadertype &s) {
  auto &shader = static_cast<fixedshadertype&>(s);
  if (rules&FIXED_KEYFRAME) {
    OGL(BindAttribLocation, shader.program, ATTRIB_POS0, "vs_pos0");
    OGL(BindAttribLocation, shader.program, ATTRIB_POS1, "vs_pos1");
  } else
    OGL(BindAttribLocation, shader.program, ATTRIB_POS0, "vs_pos");
  if (rules&FIXED_DIFFUSETEX)
    OGL(BindAttribLocation, shader.program, ATTRIB_TEX0, "vs_tex");
  if (rules&FIXED_COLOR)
    OGL(BindAttribLocation, shader.program, ATTRIB_COL, "vs_col");
#if !defined(__WEBGL__)
  OGL(BindFragDataLocation, shader.program, 0, "rt_col");
#endif // __WEBGL__
}

static void buildfixedshaders(bool fatalerr) {
  loopi(fixedshadernum) {
    auto b = NEW(fixedshaderbuilder, BUILDER_ARGS(fixed), i);
    if (!b->build(shaders[i], shaderfromfile))
      shadererror(fatalerr, "fixed shader");
  }
}

/*--------------------------------------------------------------------------
 - base rendering functions
 -------------------------------------------------------------------------*/
void drawarrays(int mode, int first, int count) {
  OGL(DrawArrays, mode, first, count);
}
void drawelements(int mode, int count, int type, const void *indices) {
  OGL(DrawElements, mode, count, type, indices);
}
static u32 coretexarray[TEX_PREALLOCATED_NUM];
u32 coretex(u32 index) { return coretexarray[index%TEX_PREALLOCATED_NUM]; }

static u32 buildcheckboard() {
  const u32 dim = 16;
  u32 *cb = (u32*)MALLOC(dim*dim*sizeof(u32));
  loopi(dim) loopj(dim) cb[i*dim+j]=(i==0)||(j==0)||(i==dim-1)||(j==dim-1)?0:~0;
  u32 id = maketex("TB I4 D4 B2 G Wsr Wtr mM Ml", cb, 16, 16);
  FREE(cb);
  return id;
}

void start(int w, int h) {
  startgl();
  OGL(Viewport, 0, 0, w, h);

#if defined (__WEBGL__)
  OGL(ClearDepthf,1.f);
#else
  OGL(ClearDepth,1.f);
#endif // __WEBGL__
  enablev(GL_DEPTH_TEST, GL_CULL_FACE);
  OGL(DepthFunc, GL_LESS);
  OGL(CullFace, GL_BACK);
  OGL(GetIntegerv, GL_MAX_TEXTURE_SIZE, &glmaxtexsize);
  dirty.any = ~0x0;
  buildfixedshaders(true);
  text::start();
  imminit();
  loopi(ATTRIB_NUM) enabledattribarray[i] = 0;
  loopi(BUFFER_NUM) bindedvbo[i] = 0;

  coretexarray[TEX_UNUSED]       = 0;
  coretexarray[TEX_CROSSHAIR]    = installtex("data/crosshair.png");
  coretexarray[TEX_CHARACTERS]   = text::oglfont();
  coretexarray[TEX_CHECKBOARD]   = buildcheckboard();
  coretexarray[TEX_MARTIN_BASE]  = installtex("data/martin/base.png");
  coretexarray[TEX_ITEM]         = installtex("data/items.png");
  coretexarray[TEX_EXPLOSION]    = installtex("data/explosion.jpg");
  coretexarray[TEX_MARTIN_BALL1] = installtex("data/martin/ball1.png");
  coretexarray[TEX_MARTIN_SMOKE] = installtex("data/martin/smoke.png");
  coretexarray[TEX_MARTIN_BALL2] = installtex("data/martin/ball2.png");
  coretexarray[TEX_MARTIN_BALL3] = installtex("data/martin/ball3.png");
  rangei(TEX_CROSSHAIR, TEX_PREALLOCATED_NUM)
    if (coretexarray[i] == 0)
      sys::fatal("could not find core textures");
}

#if !defined(RELEASE)
void finish() {
  loopi(fixedshadernum) destroyshader(shaders[i]);
  loopv(allshaders) DEL(allshaders[i].first);
  allshaders.destroy();
  rangei(TEX_CROSSHAIR, TEX_PREALLOCATED_NUM)
    if (coretexarray[i]) ogl::deletetextures(1, coretexarray+i);
  immdestroy();
  if (programnum) printf("ogl: %d shaders are still allocated\n", programnum);
  if (texturenum) printf("ogl: %d textures are still allocated\n", texturenum);
  if (buffernum) printf("ogl: %d buffers are still allocated\n", buffernum);
  if (framebuffernum) printf("ogl: %d frame buffers are still allocated\n", framebuffernum);
  cleanuptimers();
}
#endif
} /* namespace ogl */
} /* namespace q */

