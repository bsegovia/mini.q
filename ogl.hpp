/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ogl.hpp -> exposes opengl routines
 -------------------------------------------------------------------------*/
#pragma once
#include "math.hpp"
#include "sys.hpp"

namespace q {
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
void end();

// vertex attributes
enum {POS0, POS1, TEX0, TEX1, TEX2, NOR, COL, ATTRIB_NUM};

// pre-allocated texture
enum {
  TEX_CROSSHAIR = 1,
  TEX_CHARACTERS,
  TEX_MARTIN_BASE,
  TEX_ITEM,
  TEX_EXPLOSION,
  TEX_MARTIN_BALL1,
  TEX_MARTIN_SMOKE,
  TEX_MARTIN_BALL2,
  TEX_MARTIN_BALL3,
  TEX_PREALLOCATED_NUM = TEX_MARTIN_BALL3
};

// quick, dirty and super simple uber-shader system
static const u32 COLOR = 0;
static const u32 FOG = 1<<0;
static const u32 KEYFRAME = 1<<1;
static const u32 DIFFUSETEX = 1<<2;
static const int subtypen = 3;
static const int shadern = 1<<subtypen;
void bindshader(u32 flags);

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
void bindgametexture(u32 target, u32 tex);
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
void immdrawarrays(int mode, int first, int count);
void immdraw(int mode, int pos, int tex, int col, size_t n, const float *data);

// matrix interface
enum {MODELVIEW, PROJECTION, MATRIX_MODE};
void matrixmode(int mode);
void identity(void);
void rotate(float angle, const vec3f &axis);
void perspective(const mat4x4f &m, float fovy, float aspect, float znear, float zfar);
void translate(const vec3f &v);
void mulmatrix(const mat4x4f &m);
void pushmatrix(void);
void popmatrix(void);
void loadmatrix(const mat4x4f &m);
void ortho(float left, float right, float bottom, float top, float znear, float zfar);
void scale(const vec3f &s);
const mat4x4f &matrix(int mode);

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

