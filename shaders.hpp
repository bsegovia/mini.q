/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.hpp -> exposes shader source code
 -------------------------------------------------------------------------*/
namespace q {
namespace shaders {

#define DEBUG_UNSPLIT 1
#if DEBUG_UNSPLIT
extern const char debugunsplit_vp[], debugunsplit_fp[];
#endif
extern const char deferred_vp[], deferred_fp[];
extern const char fixed_vp[], fixed_fp[];
extern const char font_fp[];
extern const char dfrm_fp[];

} /* namespace shaders */
} /* namespace q */

