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

#include <SDL/SDL_image.h>

namespace q {
namespace ogl {

#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#define OGLPROC(FIELD,NAME,PROTOTYPE) PROTOTYPE FIELD = NULL;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */
static void *getfunction(const char *name) {
  void *ptr = SDL_GL_GetProcAddress(name);
  if (ptr == NULL) sys::fatal("opengl 2 is required");
  return ptr;
}

/*-------------------------------------------------------------------------
 - very simple state tracking
 -------------------------------------------------------------------------*/
union {
  struct {
    u32 shader:1; // will force to reload everything
    u32 mvp:1;
    u32 fog:1;
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
static s32 texturenum = 0, buffernum = 0, programnum = 0;

void gentextures(s32 n, u32 *id) {
  texturenum += n;
  OGL(GenTextures, n, id);
}
void deletetextures(s32 n, u32 *id) {
  texturenum -= n;
  if (texturenum < 0) sys::fatal("textures already freed");
  OGL(DeleteTextures, n, id);
}
void genbuffers(s32 n, u32 *id) {
  buffernum += n;
  OGL(GenBuffers, n, id);
}
void deletebuffers(s32 n, u32 *id) {
  buffernum -= n;
  if (buffernum < 0) sys::fatal("buffers already freed");
  OGL(DeleteBuffers, n, id);
}

static u32 createprogram(void) {
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
void identity(void) {
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
void pushmatrix(void) {
  assert(vpdepth+1<MATRIX_STACK);
  vpstack[vpdepth++][vpmode] = vp[vpmode];
}
void popmatrix(void) {
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

INLINE bool ispoweroftwo(unsigned int x) { return ((x&(x-1))==0); }

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
        }
        ogl::bindtexture(target, id, 0);
        OGL(PixelStorei, GL_UNPACK_ALIGNMENT, 1);
        data = va_arg(args, void*);
        loopi(dimnum) dim[i] = va_arg(args, u32);
        if (target == GL_TEXTURE_1D)
          OGL(TexImage1D, target, 0, internalfmt, dim[0], 0, datafmt, type, data);
        else if (target == GL_TEXTURE_2D)
          OGL(TexImage2D, target, 0, internalfmt, dim[0], dim[1], 0, datafmt, type, data);
        else if (target == GL_TEXTURE_3D)
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
      }
      break;
      case 'I': PEEK { // internal data format
        case '3': internalfmt = GL_RGB; break;
        case '4': internalfmt = GL_RGBA; break;
        case 'r': internalfmt = GL_RED; break;
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

static void imminit(void) {
  initbuffer(bigvbo, ARRAY_BUFFER, immbuffersize);
  initbuffer(bigibo, ELEMENT_ARRAY_BUFFER, immbuffersize);
  memset(immattribs, 0, sizeof(immattribs));
}

void immattrib(int attrib, int n, int type, int offset) {
  immattribs[attrib].n = n;
  immattribs[attrib].type = type;
  immattribs[attrib].offset = offset;
}

void immvertexsize(int sz) { immvertexsz = sz; }

static void immsetallattribs(void) {
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
  drawarrays(mode,first,count);
}

void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices) {
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
  const void *fake = (const void *) intptr_t(drawibooffset);
  drawelements(mode, count, type, fake);
}

void immdrawelememts(const char *fmt, int count, const void *indices, const void *vertices) {
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
      ATTRIB('p', POS0);
      ATTRIB('t', TEX0);
      ATTRIB('c', COL);
#undef ATTRIB
    }
    ++fmt;
  }
  immvertexsize(offset);
  immdrawelements(mode, count, type, indices, vertices);
}

void immdraw(int mode, int pos, int tex, int col, size_t n, const float *data) {
  const int sz = (pos+tex+col)*sizeof(float);
  if (!immvertices(n*sz, data)) return;
  loopi(ATTRIB_NUM) disableattribarray(i);
  if (pos) {
    immattrib(ogl::POS0, pos, GL_FLOAT, (tex+col)*sizeof(float));
    enableattribarray(POS0);
  } else
    disableattribarray(POS0);
  if (tex) {
    immattrib(ogl::TEX0, tex, GL_FLOAT, col*sizeof(float));
    enableattribarray(TEX0);
  } else
    disableattribarray(TEX0);
  if (col) {
    immattrib(ogl::COL, col, GL_FLOAT, 0);
    enableattribarray(COL);
  } else
    disableattribarray(COL);
  immvertexsize(sz);
  immsetallattribs();
  immdrawarrays(mode, 0, n);
}

/*--------------------------------------------------------------------------
 - quick and dirty shader management
 -------------------------------------------------------------------------*/
static bool checkshader(const char *source, const char *rules, u32 shadernumame) {
  GLint result = GL_FALSE;
  int infologlength;

  if (!shadernumame) return false;
  OGL(GetShaderiv, shadernumame, GL_COMPILE_STATUS, &result);
  OGL(GetShaderiv, shadernumame, GL_INFO_LOG_LENGTH, &infologlength);
  if (infologlength > 1) {
    char *buffer = (char*) malloc(infologlength+1);
    buffer[infologlength] = 0;
    OGL(GetShaderInfoLog, shadernumame, infologlength, NULL, buffer);
    con::out("%s", buffer);
    printf("in\n%s%s\n", rules, source);
    free(buffer);
  }
  return result == GL_TRUE;
}

static u32 loadshader(GLenum type, const char *source, const char *rulestr) {
  u32 name;
  OGLR(name, CreateShader, type);
  const char *sources[] = {rulestr, source};
  OGL(ShaderSource, name, 2, sources, NULL);
  OGL(CompileShader, name);
  if (!checkshader(source, rulestr, name)) return 0;
  return name;
}

#if defined(__WEBGL__)
  #define OGL_PROGRAM_HEADER\
  "precision highp float;\n"\
  "#define IF_WEBGL(X) X\n"\
  "#define IF_NOT_WEBGL(X)\n"\
  "#define SWITCH_WEBGL(X,Y) X\n"\
  "#define VS_IN attribute\n"\
  "#define VS_OUT varying\n"\
  "#define PS_IN varying\n"
#else
#define OGL_PROGRAM_HEADER\
  "#version 130\n"\
  "#define IF_WEBGL(X)\n"\
  "#define IF_NOT_WEBGL(X) X\n"\
  "#define SWITCH_WEBGL(X,Y) Y\n"\
  "#define VS_IN in\n"\
  "#define VS_OUT out\n"\
  "#define PS_IN in\n"
#endif // __WEBGL__

static u32 loadprogram(const char *vertstr, const char *fragstr, u32 rules) {
  u32 program = 0;
  sprintf_sd(rulestr)(OGL_PROGRAM_HEADER
                      "#define USE_COL %i\n"
                      "#define USE_FOG %i\n"
                      "#define USE_KEYFRAME %i\n"
                      "#define USE_DIFFUSETEX %i\n",
                      rules&COLOR,rules&FOG,rules&KEYFRAME,rules&DIFFUSETEX);
  const u32 vert = loadshader(GL_VERTEX_SHADER, vertstr, rulestr);
  const u32 frag = loadshader(GL_FRAGMENT_SHADER, fragstr, rulestr);
  if (vert == 0 || frag == 0) return 0;
  program = createprogram();
  OGL(AttachShader, program, vert);
  OGL(AttachShader, program, frag);
  OGL(DeleteShader, vert);
  OGL(DeleteShader, frag);
  return program;
}
#undef OGL_PROGRAM_HEADER

static struct shadertype {
  u32 rules; // fog,keyframe...?
  u32 program; // ogl program
  u32 u_diffuse, u_delta, u_mvp; // uniforms
  u32 u_zaxis, u_fogstartend, u_fogcolor; // uniforms
} shaders[shadernum];
static shadertype fontshader;
static vec4f fogcolor;
static vec2f fogstartend;

static void bindshader(shadertype &shader) {
  if (bindedshader != &shader) {
    bindedshader = &shader;
    dirty.any = ~0x0;
    OGL(UseProgram, bindedshader->program);
  }
}

void bindfixedshader(u32 flags) { bindshader(shaders[flags]); }

static void linkshader(shadertype &shader) {
  OGL(LinkProgram, shader.program);
  OGL(ValidateProgram, shader.program);
}
void setshaderudelta(float delta) { // XXX find a better way to handle this!
  if (bindedshader) OGL(Uniform1f, bindedshader->u_delta, delta);
}

static void setshaderuniform(shadertype &shader) {
  OGL(UseProgram, shader.program);
  OGLR(shader.u_mvp, GetUniformLocation, shader.program, "u_mvp");
  if (shader.rules&KEYFRAME)
    OGLR(shader.u_delta, GetUniformLocation, shader.program, "u_delta");
  else
    shader.u_delta = 0;
  if (shader.rules&DIFFUSETEX) {
    OGLR(shader.u_diffuse, GetUniformLocation, shader.program, "u_diffuse");
    OGL(Uniform1i, shader.u_diffuse, 0);
  }
  if (shader.rules&FOG) {
    OGLR(shader.u_zaxis, GetUniformLocation, shader.program, "u_zaxis");
    OGLR(shader.u_fogcolor, GetUniformLocation, shader.program, "u_fogcolor");
    OGLR(shader.u_fogstartend, GetUniformLocation, shader.program, "fogstartend");
  }
  OGL(UseProgram, 0);
}

static bool compileshader(shadertype &shader, const char *vert, const char *frag, u32 rules) {
  memset(&shader, 0, sizeof(shadertype));
  if ((shader.program = loadprogram(vert, frag, rules))==0) return false;
  shader.rules = rules;
  if (shader.rules&KEYFRAME) {
    OGL(BindAttribLocation, shader.program, POS0, "vs_pos0");
    OGL(BindAttribLocation, shader.program, POS1, "vs_pos1");
  } else
    OGL(BindAttribLocation, shader.program, POS0, "vs_pos");
  if (shader.rules&DIFFUSETEX)
    OGL(BindAttribLocation, shader.program, TEX0, "vs_tex");
  OGL(BindAttribLocation, shader.program, COL, "vs_col");
#if !defined(__WEBGL__)
  OGL(BindFragDataLocation, shader.program, 0, "rt_col");
#endif // __WEBGL__
  return true;
}

typedef void (CDECL *uniformcb)(shadertype&);
static bool buildprogram(shadertype &shader, const char *vert, const char *frag, u32 rules, uniformcb cb = NULL) {
  shadertype s;
  if (!compileshader(s, vert, frag, rules)) return false;
  if (shader.program) deleteprogram(shader.program);
  // XXX we handle in a nasty way different shader size. I guess we should not
  // make the copy like this
  shader = s;
  linkshader(shader);
  setshaderuniform(shader);
  if (cb) cb(shader);
  dirty.any = ~0x0;
  return true;
}

static char *loadshaderfile(const char *path) {
  char *s = sys::loadfile(path);
  if (s == NULL) con::out("unable to load shader %s", path);
  return s;
}

struct shaderresource {
  const char *vppath, *fppath;
  const char *vp, *fp;
  uniformcb cb;
};
static const struct shaderresource fixed_rsc = {
  "data/shaders/fixed_vp.glsl",
  "data/shaders/fixed_fp.glsl",
  shaders::fixed_vp,
  shaders::fixed_fp,
  NULL
};
static const struct shaderresource font_rsc = {
  "data/shaders/fixed_vp.glsl",
  "data/shaders/font_fp.glsl",
  shaders::fixed_vp,
  shaders::font_fp,
  NULL
};

static struct dfrmshadertype : shadertype {
  u32 u_globaltime, u_iresolution;
} dfrmshader;
static void dfrmuniform(shadertype &s) {
  auto &df = static_cast<dfrmshadertype&>(s);
  OGLR(df.u_globaltime, GetUniformLocation, df.program, "iGlobalTime");
  OGLR(df.u_iresolution, GetUniformLocation, df.program, "iResolution");
}
static const struct shaderresource dfrm_rsc = {
  "data/shaders/fixed_vp.glsl",
  "data/shaders/dfrm_fp.glsl",
  shaders::fixed_vp,
  shaders::dfrm_fp,
  dfrmuniform
};

void bindshader(u32 idx) {
  if (idx == FONT_SHADER)
    bindshader(fontshader);
  else
    bindshader(dfrmshader);
}

static bool buildprogramfromfile(shadertype &s, const shaderresource &rsc, u32 rules) {
  auto fixed_vp = loadshaderfile(rsc.vppath);
  auto fixed_fp = loadshaderfile(rsc.fppath);
  if (fixed_fp == NULL || fixed_vp == NULL) return false;
  auto ret = buildprogram(s, fixed_vp, fixed_fp, rules, rsc.cb);
  free(fixed_fp);
  free(fixed_vp);
  return ret;
}
static bool buildshader(shadertype &s, const shaderresource &rsc, u32 rules, int fromfile) {
  if (fromfile)
    return buildprogramfromfile(s, rsc, rules);
  else
    return buildprogram(s, rsc.vp, rsc.fp, rules, rsc.cb);
}
IVAR(shaderfromfile, 0, 1, 1);

static void shadererror(bool fatalerr, const char *msg) {
  if (fatalerr)
    sys::fatal("unable to build fixed shaders %s", msg);
  else
    con::out("unable to build fixed shaders %s", msg);
}
static void buildshaders(bool fatalerr) {
  loopi(shadernum) if (!buildshader(shaders[i], fixed_rsc, i, shaderfromfile))
    shadererror(fatalerr, "fixed shader");
  if (!buildshader(fontshader, font_rsc, DIFFUSETEX, shaderfromfile))
    shadererror(fatalerr, "font shader");
  if (!buildshader(dfrmshader, dfrm_rsc, 0, shaderfromfile))
    shadererror(fatalerr, "distance field shader");
}

static void destroyshaders() {
  loopi(shadernum) deleteprogram(shaders[i].program);
  deleteprogram(fontshader.program);
  deleteprogram(dfrmshader.program);
}
static void reloadshaders() { buildshaders(false); }
CMD(reloadshaders, "");

/*--------------------------------------------------------------------------
 - base rendering functions
 -------------------------------------------------------------------------*/
// flush all the states required for the draw call
static void flush(void) {
  if (dirty.any == 0) return; // fast path
  if (dirty.flags.shader) {
    OGL(UseProgram, bindedshader->program);
    dirty.flags.shader = 0;
  }
  if ((bindedshader->rules & FOG) && (dirty.flags.fog || dirty.flags.mvp)) {
    const mat4x4f &mv = vp[MODELVIEW];
    const vec4f zaxis(mv.vx.z,mv.vy.z,mv.vz.z,mv.vw.z);
    OGL(Uniform2fv, bindedshader->u_fogstartend, 1, &fogstartend.x);
    OGL(Uniform4fv, bindedshader->u_fogcolor, 1, &fogcolor.x);
    OGL(Uniform4fv, bindedshader->u_zaxis, 1, &zaxis.x);
    dirty.flags.fog = 0;
  }
  if (dirty.flags.mvp) {
    viewproj = vp[PROJECTION]*vp[MODELVIEW];
    OGL(UniformMatrix4fv, bindedshader->u_mvp, 1, GL_FALSE, &viewproj.vx.x);
    dirty.flags.mvp = 0;
  }
  // XXX HACK!
  if (bindedshader == &dfrmshader) {
    auto dfrm = static_cast<dfrmshadertype*>(bindedshader);
    const vec3f ires(float(sys::scrw), float(sys::scrh), 1.f);
    OGL(Uniform3fv, dfrm->u_iresolution, 1, &ires.x);
    OGL(Uniform1f, dfrm->u_globaltime, 1e-3f*game::lastmillis);
  }
}
void drawarrays(int mode, int first, int count) {
  flush();
  OGL(DrawArrays, mode, first, count);
}
void drawelements(int mode, int count, int type, const void *indices) {
  flush();
  OGL(DrawElements, mode, count, type, indices);
}
static u32 coretexarray[TEX_PREALLOCATED_NUM];
u32 coretex(u32 index) { return coretexarray[index%TEX_PREALLOCATED_NUM]; }

static u32 buildcheckboard() {
  const u32 dim = 16;
  u32 *cb = (u32*)malloc(dim*dim*sizeof(u32));
  loopi(dim) loopj(dim) cb[i*dim+j]=(i==0)||(j==0)||(i==dim-1)||(j==dim-1)?0:~0;
  u32 id = maketex("TB I4 D4 B2 G Wsr Wtr mM Ml", cb, 16, 16);
  free(cb);
  return id;
}

void start(int w, int h) {
#if !defined(__WEBGL__)
// on windows, we directly load OpenGL 1.1 functions
#if defined(__WIN32__)
  #define OGLPROC110(FIELD,NAME,PROTOTYPE) FIELD = (PROTOTYPE) NAME;
#else
  #define OGLPROC110 OGLPROC
#endif /* __WIN32__ */
#define OGLPROC(FIELD,NAME,PROTO) FIELD = (PROTO) getfunction(#NAME);
#include "ogl.hxx"
#undef OGLPROC110
#undef OGLPROC
#endif /* __WEBGL__ */
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
  buildshaders(true);
  imminit();
  loopi(ATTRIB_NUM) enabledattribarray[i] = 0;
  loopi(BUFFER_NUM) bindedvbo[i] = 0;

  coretexarray[TEX_UNUSED]       = 0;
  coretexarray[TEX_CROSSHAIR]    = installtex("data/crosshair.png");
  coretexarray[TEX_CHARACTERS]   = text::buildfont();
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
void end() {
  destroyshaders();
  rangei(TEX_CROSSHAIR, TEX_PREALLOCATED_NUM)
    if (coretexarray[i]) OGL(DeleteTextures, 1, coretexarray+i);
}

} /* namespace ogl */
} /* namespace q */

