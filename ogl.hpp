/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ogl.hpp -> exposes opengl routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"

namespace q {
template<typename T> struct vec3;
template<typename T> struct mat4x4;
namespace ogl {

// declare all GL functions
#if !defined(__WEBGL__)
#define OGLPROC110(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#define OGLPROC(FIELD,NAME,PROTOTYPE) extern PROTOTYPE FIELD;
#include "ogl.hxx"
#undef OGLPROC
#undef OGLPROC110
#endif /* __WEBGL__ */

void start(int w, int h);
void finish();

// vertex attributes
enum {POS0, POS1, TEX0, TEX1, TEX2, TEX3, NOR, COL, ATTRIB_NUM};

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

// track allocations
void gentextures(s32 n, u32 *id);
void genbuffers(s32 n, u32 *id);
void deletetextures(s32 n, u32 *id);
void deletebuffers(s32 n, u32 *id);

// draw helper functions
void draw(int mode, int pos, int tex, size_t n, const float *data);
void drawarrays(int mode, int first, int count);
void drawelements(int mode, int count, int type, const void *indices);
void rendermd2(const float *pos0, const float *pos1, float lerp, int n);
void drawsphere(void);

// following functions also ensure state tracking
enum {ARRAY_BUFFER, ELEMENT_ARRAY_BUFFER, BUFFER_NUM};
void bindbuffer(u32 target, u32 buffer);
void bindtexture(u32 target, u32 id, u32 slot=0);
void enableattribarray(u32 target);
void disableattribarray(u32 target);

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

// immediate mode rendering
void immvertexsize(int sz);
void immattrib(int attrib, int n, int type, int offset);
void immdrawelements(int mode, int count, int type, const void *indices, const void *vertices);
void immdrawelememts(const char *fmt, int count, const void *indices, const void *vertices);
void immdrawarrays(int mode, int first, int count);
void immdraw(int mode, int pos, int tex, int col, size_t n, const float *data);

// matrix interface
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

// basic shader type
struct shadertype {
  INLINE shadertype() : rules(0), program(0) {}
  u32 rules; // fog,keyframe...?
  u32 program; // ogl program
  u32 u_diffuse, u_delta, u_mvp; // uniforms
  u32 u_zaxis, u_fogstartend, u_fogcolor; // uniforms
};
void destroyshader(shadertype &s);

// quick, dirty and super simple shader system to replace fixed pipeline
static const u32 FOG = 1<<0;
static const u32 KEYFRAME = 1<<1;
static const u32 DIFFUSETEX = 1<<2;
static const u32 COLOR = 1<<3;
static const int subtypenum = 4;
static const int shadernum = 1<<subtypenum;
void bindfixedshader(u32 flags);
void bindshader(shadertype&);
void setshaderudelta(float delta); // XXX find a better way to handle this!

// to create shaders with any extra uniforms
typedef void (CDECL *uniformcb)(shadertype&);
struct shaderresource {
  const char *vppath, *fppath;
  const char *vp, *fp;
  uniformcb cb;
};
bool buildshader(shadertype &s, const shaderresource &rsc, u32 rules, int fromfile);
void shadererror(bool fatalerr, const char *msg);
bool loadfromfile();

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

// useful to enable / disable lot of stuff in one-liners
INLINE void enable(u32 x) { OGL(Enable,x); }
INLINE void disable(u32 x) { OGL(Disable,x); }
MAKE_VARIADIC(enable);
MAKE_VARIADIC(disable);

} /* namespace ogl */
} /* namespace q */

