/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.hpp -> exposes shader source code
 -------------------------------------------------------------------------*/
namespace q {
namespace shaders {

#define DEBUG_UNSPLIT 1
#define SHADER(NAME)\
extern const char NAME##_vp[], NAME##_fp[];\
extern const char NAME##_vp_decl[], NAME##_fp_decl[];
SHADER(forward);
SHADER(deferred);
SHADER(debugunsplit);
#undef SHADER
extern const char macros[];
extern const char noise2D[], noise3D[], noise4D[];
extern const char fixed_vp[], fixed_fp[];
extern const char font_fp[];
extern const char dfrm_fp[];

} /* namespace shaders */
} /* namespace q */

