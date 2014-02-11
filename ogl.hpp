/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ogl.hpp -> exposes opengl routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "stl.hpp"

namespace q {
template<typename T> struct vec3;
template<typename T> struct mat4x4;
namespace ogl {

/*--------------------------------------------------------------------------
 - various helper and core routines
 -------------------------------------------------------------------------*/
void start(int w, int h);
void finish();

// declare all GL functions
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#define OGLPROC(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */

// vertex attributes
enum {
  ATTRIB_POS0,
  ATTRIB_POS1,
  ATTRIB_TEX0,
  ATTRIB_TEX1,
  ATTRIB_TEX2,
  ATTRIB_TEX3,
  ATTRIB_NOR,
  ATTRIB_COL,
  ATTRIB_NUM
};

// like OGL versions but also track allocations
void gentextures(s32 n, u32 *id);
void deletetextures(s32 n, u32 *id);
void genbuffers(s32 n, u32 *id);
void deletebuffers(s32 n, u32 *id);
void genframebuffers(s32 n, u32 *id);
void deleteframebuffers(s32 n, u32 *id);

// following functions also ensure state tracking
enum {ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER, BUFFER_NUM};
void bindbuffer(u32 target, u32 buffer);
void bindtexture(u32 target, u32 id, u32 slot=0);
void enableattribarray(u32 target);
void disableattribarray(u32 target);

// draw helper functions
void drawarrays(int mode, int first, int count);
void drawelements(int mode, int count, int type, const void *indices);
void rendermd2(const float *pos0, const float *pos1, float lerp, int n);
void drawsphere(void);

// enable /disable vertex attribs in one shot
struct setattribarray {
  INLINE setattribarray(void) {
    loopi(ATTRIB_NUM) enabled[i] = false;
  }
  template <typename First, typename... Rest>
  INLINE void operator() (First first, Rest... rest) {
    enabled[first] = true;
    operator() (rest...);
  }
  INLINE void operator()() {
    loopi(ATTRIB_NUM) if (enabled[i]) enableattribarray(i); else disableattribarray(i);
  }
  bool enabled[ATTRIB_NUM];
};

// OGL debug macros
#if !defined(__EMSCRIPTEN__)
#if !defined(NDEBUG)
#define OGL(NAME, ...) \
  do { \
    ogl::NAME(__VA_ARGS__); \
    if (q::ogl::GetError()) sys::fatal("gl" #NAME " failed"); \
  } while (0)
#define OGLR(RET, NAME, ...) \
  do { \
    RET = q::ogl::NAME(__VA_ARGS__); \
    if (q::ogl::GetError()) sys::fatal("gl" #NAME " failed"); \
  } while (0)
#else
  #define OGL(NAME, ...) do {q::ogl::NAME(__VA_ARGS__);} while(0)
  #define OGLR(RET, NAME, ...) do {RET=q::ogl::NAME(__VA_ARGS__);} while(0)
#endif /* NDEBUG */
#else /* __EMSCRIPTEN__ */
#if !defined(NDEBUG)
#define OGL(NAME, ...) \
  do { \
    gl##NAME(__VA_ARGS__); \
    if (gl##GetError()) { \
      sprintf_sd(err)("gl" #NAME " failed at line %i and file %s",__LINE__, __FILE__);\
      sys::fatal(err); \
    } \
  } while (0)
#define OGLR(RET, NAME, ...) \
  do { \
    RET = gl##NAME(__VA_ARGS__); \
    if (gl##GetError()) sys::fatal("gl" #NAME " failed"); \
  } while (0)
#else
  #define OGL(NAME, ...) do {gl##NAME(__VA_ARGS__);} while(0)
  #define OGLR(RET, NAME, ...) do {RET=gl##NAME(__VA_ARGS__);} while(0)
#endif /* NDEBUG */
#endif /* __EMSCRIPTEN__ */

// useful to enable / disable many things in one-liners
INLINE void enable(u32 x) { OGL(Enable,x); }
INLINE void disable(u32 x) { OGL(Disable,x); }
MAKE_VARIADIC(enable);
MAKE_VARIADIC(disable);

/*--------------------------------------------------------------------------
 - texture stuff
 -------------------------------------------------------------------------*/
// pre-allocated texture
enum {
  TEX_UNUSED = 0,
  TEX_CROSSHAIR,
  TEX_CHARACTERS,
  TEX_CHECKBOARD,
  TEX_MARTIN_BASE,
  TEX_ITEM,
  TEX_EXPLOSION,
  TEX_MARTIN_BALL1,
  TEX_MARTIN_SMOKE,
  TEX_MARTIN_BALL2,
  TEX_MARTIN_BALL3,
  TEX_PREALLOCATED_NUM
};
u32 coretex(u32 index);
u32 installtex(const char *texname, bool clamp=false);
u32 maketex(const char *fmt, ...);

/*--------------------------------------------------------------------------
 - immediate mode rendering
 -------------------------------------------------------------------------*/
void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices);
void immdrawelememts(const char *fmt, int count, const void *indices, const void *vertices);
void immdrawarrays(int mode, int first, int count);
void immdraw(const char *fmt, int count, const void *data);

/*--------------------------------------------------------------------------
 - matrix interface similar to OpenGL 1.x
 -------------------------------------------------------------------------*/
enum {MODELVIEW, PROJECTION, MATRIX_MODE};
void matrixmode(int mode);
void identity(void);
void rotate(float angle, const vec3<float> &axis);
void translate(const vec3<float> &v);
void mulmatrix(const mat4x4<float> &m);
void pushmatrix(void);
void popmatrix(void);
void loadmatrix(const mat4x4<float> &m);
void setperspective(float fovy, float aspect, float znear, float zfar);
void setortho(float left, float right, float bottom, float top, float znear, float zfar);
void scale(const vec3<float> &s);
const mat4x4<float> &matrix(int mode);
INLINE void pushmode(int mode) {
  ogl::matrixmode(mode);
  ogl::pushmatrix();
}
INLINE void popmode(int mode) {
  ogl::matrixmode(mode);
  ogl::popmatrix();
}

/*--------------------------------------------------------------------------
 - basic shader type
 -------------------------------------------------------------------------*/
struct shadertype {
  INLINE shadertype(bool fixedfunction=false) :
    program(0), fixedfunction(fixedfunction) {}
  u32 program;
  bool fixedfunction;
};
void destroyshader(shadertype&);
void bindshader(shadertype&);

// provides rules source when compiling a shader
typedef vector<char*> shaderrules;

// to be overloaded when creating shaders
struct shaderbuilder {
  shaderbuilder(const char *vppath, const char *fppath,
                const char *vp, const char *fp) :
    vppath(vppath), fppath(fppath), vp(vp), fp(fp) {}
  bool build(shadertype &s, int fromfile, bool save = true);
private:
  const char *vppath, *fppath;
  const char *vp, *fp;
  virtual void setrules(shaderrules &vertrules, shaderrules &fragrules) = 0;
  virtual void setuniform(shadertype &s) = 0;
  virtual void setinout(shadertype &s) = 0;
  u32 compile(const char *vertsrc, const char *fragsrc);
  bool buildprogram(shadertype &s, const char *vert, const char *frag);
  bool buildprogramfromfile(shadertype &s);
};
#define BUILDER_ARGS(X) \
"data/shaders/" #X "_vp.glsl", \
"data/shaders/" #X "_fp.glsl", \
shaders:: X##_vp, \
shaders:: X##_fp

void shadererror(bool fatalerr, const char *msg);
bool loadfromfile();

/*--------------------------------------------------------------------------
 - timer functions to measure performance on CPU/GPU
 -------------------------------------------------------------------------*/
void beginframe();
void endframe();
struct timer *begintimer(const char *name, bool gpu);
void endtimer(struct timer *t);
void printtimers(float conw, float conh);

/*--------------------------------------------------------------------------
 - simple shader system to replace fixed pipeline
 -------------------------------------------------------------------------*/
static const u32 FIXED_KEYFRAME = 1<<0;
static const u32 FIXED_DIFFUSETEX = 1<<1;
static const u32 FIXED_COLOR = 1<<2;
static const int fixedsubtypenum = 3;
static const int fixedshadernum = 1<<fixedsubtypenum;
void bindfixedshader(u32 flags);
void bindfixedshader(u32 flags, float delta);
void fixedflush();

// can be reused for similar shaders
struct fixedshadertype : shadertype {
  fixedshadertype() : shadertype(true), rules(0) {}
  u32 rules; // flags to enable / disable features
  u32 u_diffuse, u_delta, u_mvp; // uniforms
};

// can be reused by other shaders reusing the same skeleton as fixed shaders
struct fixedshaderbuilder : shaderbuilder {
  fixedshaderbuilder(const char *vppath, const char *fppath,
                     const char *vp, const char *fp, u32 rules);
  virtual void setrules(shaderrules&, shaderrules&);
  virtual void setuniform(shadertype &s);
  virtual void setinout(shadertype &s);
  u32 rules;
};
} /* namespace ogl */
} /* namespace q */

