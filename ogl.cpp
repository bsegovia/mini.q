/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ogl.cpp -> defines opengl routines
 -------------------------------------------------------------------------*/
#include "ogl.hpp"
#include "con.hpp"
#include "sys.hpp"
#include <SDL/SDL_image.h>

namespace q {
namespace ogl {

#if defined(__JAVASCRIPT__)
#define __WEBGL__
#define IF_WEBGL(X) X
#define IF_NOT_WEBGL(X)
#define SWITCH_WEBGL(X,Y) X
#else
#define IF_WEBGL(X)
#define IF_NOT_WEBGL(X) X
#define SWITCH_WEBGL(X,Y) Y
#endif //__JAVASCRIPT__

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
static struct shader *bindedshader = NULL;

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
void perspective(float fovy, float aspect, float znear, float zfar) {
  dirty.flags.mvp=1;
  vp[vpmode] = vp[vpmode]*q::perspective(fovy,aspect,znear,zfar);
}
void ortho(float left, float right, float bottom, float top, float znear, float zfar) {
  dirty.flags.mvp=1;
  vp[vpmode] = vp[vpmode]*q::ortho(left,right,bottom,top,znear,zfar);
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

u32 installtex(const char *texname, bool clamp) {
  SDL_Surface *s = IMG_Load(texname);
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

  u32 id;
  gentextures(1, &id);
  loopi(int(TEX_NUM)) bindedtexture[i] = 0;
  con::out("loading %s (%ix%i)", texname, s->w, s->h);
  ogl::bindtexture(GL_TEXTURE_2D, id, 0);
  OGL(PixelStorei, GL_UNPACK_ALIGNMENT, 1);
  if (s->w>glmaxtexsize || s->h>glmaxtexsize)
    sys::fatal("texture dimensions are too large");
  if (s->format->BitsPerPixel == 24)
    OGL(TexImage2D, GL_TEXTURE_2D, 0, GL_RGB, s->w, s->h, 0, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
  else if (s->format->BitsPerPixel == 32)
    OGL(TexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
  else
    sys::fatal("unsupported texture format");

  if (ispoweroftwo(s->w) && ispoweroftwo(s->h)) {
    OGL(GenerateMipmap, GL_TEXTURE_2D);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  } else {
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
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

void immdrawarrays(int mode, int count, int type) {
  drawarrays(mode,count,type);
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
static bool checkshader(u32 shadername) {
  GLint result = GL_FALSE;
  int infologlength;

  if (!shadername) return false;
  OGL(GetShaderiv, shadername, GL_COMPILE_STATUS, &result);
  OGL(GetShaderiv, shadername, GL_INFO_LOG_LENGTH, &infologlength);
  if (infologlength > 1) {
    char *buffer = (char*) malloc(infologlength+1);
    buffer[infologlength] = 0;
    OGL(GetShaderInfoLog, shadername, infologlength, NULL, buffer);
    con::out("%d %s", buffer);
    free(buffer);
  }
  if (result == GL_FALSE) sys::fatal("ogl: failed to compile shader");
  return result == GL_TRUE;
}

static u32 loadshader(GLenum type, const char *source, const char *rulestr) {
  u32 name;
  OGLR(name, CreateShader, type);
  const char *sources[] = {rulestr, source};
  OGL(ShaderSource, name, 2, sources, NULL);
  OGL(CompileShader, name);
  if (!checkshader(name)) sys::fatal("OGL: shader not valid");
  return name;
}

#if defined(__WEBGL__)
#define OGL_PROGRAM_HEADER\
  "precision highp float;\n"\
  "#define VS_IN attribute\n"\
  "#define VS_OUT varying\n"\
  "#define PS_IN varying\n"
#else
#define OGL_PROGRAM_HEADER\
  "#version 130\n"\
  "#define VS_IN in\n"\
  "#define VS_OUT out\n"\
  "#define PS_IN in\n"
#endif // __WEBGL__

static u32 loadprogram(const char *vertstr, const char *fragstr, u32 rules) {
  u32 program = 0;
  sprintf_sd(rulestr)(OGL_PROGRAM_HEADER
                      "#define USE_FOG %i\n"
                      "#define USE_KEYFRAME %i\n"
                      "#define USE_DIFFUSETEX %i\n",
                      rules&FOG,rules&KEYFRAME,rules&DIFFUSETEX);
  const u32 vert = loadshader(GL_VERTEX_SHADER, vertstr, rulestr);
  const u32 frag = loadshader(GL_FRAGMENT_SHADER, fragstr, rulestr);
  program = createprogram();
  OGL(AttachShader, program, vert);
  OGL(AttachShader, program, frag);
  OGL(DeleteShader, vert);
  OGL(DeleteShader, frag);
  return program;
}
#undef OGL_PROGRAM_HEADER

static const char ubervert[] = {
  "uniform mat4 u_mvp;\n"
  "#if USE_FOG\n"
  "  uniform vec4 u_zaxis;\n"
  "  VS_OUT float fs_fogz;\n"
  "#endif\n"
  "#if USE_KEYFRAME\n"
  "  uniform float u_delta;\n"
  "  VS_IN vec3 vs_pos0, vs_pos1;\n"
  "#else\n"
  "  VS_IN vec3 vs_pos;\n"
  "#endif\n"
  "VS_IN vec4 vs_col;\n"
  "#if USE_DIFFUSETEX\n"
  "  VS_IN vec2 vs_tex;\n"
  "  VS_OUT vec2 fs_tex;\n"
  "#endif\n"
  "VS_OUT vec4 fs_col;\n"
  "void main() {\n"
  "#if USE_DIFFUSETEX\n"
  "  fs_tex = vs_tex;\n"
  "#endif\n"
  "  fs_col = vs_col;\n"
  "#if USE_KEYFRAME\n"
  "  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);\n"
  "#endif\n"
  "#if USE_FOG\n"
  "  fs_fogz = dot(u_zaxis.xyz,vs_pos)+u_zaxis.w;\n"
  "#endif\n"
  "  gl_Position = u_mvp*vec4(vs_pos,1.0);\n"
  "}\n"
};
static const char uberfrag[] = {
  "#if USE_DIFFUSETEX\n"
  "  uniform sampler2D u_diffuse;\n"
  "  PS_IN vec2 fs_tex;\n"
  "#endif\n"
  "#if USE_FOG\n"
  "  uniform vec4 u_fogcolor;\n"
  "  uniform vec2 u_fogstartend;\n"
  "  PS_IN float fs_fogz;\n"
  "#endif\n"
  "PS_IN vec4 fs_col;\n"
  IF_NOT_WEBGL("out vec4 rt_c;\n")
  "void main() {\n"
  "  vec4 col;\n"
  "#if USE_DIFFUSETEX\n"
  "  col = texture2D(u_diffuse, fs_tex);\n"
  "  col *= fs_col;\n"
  "#else\n"
  "  col = fs_col;\n"
  "#endif\n"
  "#if USE_FOG\n"
  "  float factor = clamp((-fs_fogz-u_fogstartend.x)*u_fogstartend.y,0.0,1.0)\n;"
  "  col.xyz = mix(col.xyz,u_fogcolor.xyz,factor);\n"
  "#endif\n"
  SWITCH_WEBGL("gl_FragColor = col;\n", "rt_c = col;\n")
  "}\n"
};

static struct shader {
  u32 rules; // fog,keyframe...?
  u32 program; // ogl program
  u32 u_diffuse, u_delta, u_mvp; // uniforms
  u32 u_zaxis, u_fogstartend, u_fogcolor; // uniforms
} shaders[shadern];

static vec4f fogcolor;
static vec2f fogstartend;

static void bindshader(shader &shader) {
  if (bindedshader != &shader) {
    bindedshader = &shader;
    dirty.any = ~0x0;
    OGL(UseProgram, bindedshader->program);
  }
}

void bindshader(u32 flags) { bindshader(shaders[flags]); }

static void linkshader(shader &shader) {
  OGL(LinkProgram, shader.program);
  OGL(ValidateProgram, shader.program);
}

static void setshaderuniform(shader &shader) {
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

static void compileshader(shader &shader, const char *vert, const char *frag, u32 rules) {
  memset(&shader, 0, sizeof(struct shader));
  shader.program = loadprogram(vert, frag, rules);
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
}

static void buildshader(shader &shader, const char *vert, const char *frag, u32 rules) {
  compileshader(shader, vert, frag, rules);
  linkshader(shader);
  setshaderuniform(shader);
}

static void buildubershader(shader &shader, u32 rules) {
  buildshader(shader, ubervert, uberfrag, rules);
}

static void buildshaders(void) {
  loopi(shadern) buildubershader(shaders[i], i);
}

static void destroyshaders(void) {
  loopi(shadern) deleteprogram(shaders[i].program);
}

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
  OGL(CullFace, GL_FRONT);
  OGL(GetIntegerv, GL_MAX_TEXTURE_SIZE, &glmaxtexsize);
  dirty.any = ~0x0;
  buildshaders();
  imminit();
  loopi(ATTRIB_NUM) enabledattribarray[i] = 0;
  loopi(BUFFER_NUM) bindedvbo[i] = 0;

  coretexarray[TEX_UNUSED]       = 0;
  coretexarray[TEX_CROSSHAIR]    = installtex("data/crosshair.png");
  coretexarray[TEX_CHARACTERS]   = installtex("data/newchars.png");
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
void end() { destroyshaders(); }

} /* namespace ogl */
} /* namespace q */

