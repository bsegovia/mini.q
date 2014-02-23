/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - md2.cpp -> exposes quake md2 model routines
 -------------------------------------------------------------------------*/
#include "math.hpp"

namespace q {
namespace md2 {
void start();
void finish();
void render(const char *name, int frame, int range,
            const mat4x4f &posxfm, const mat3x3f &norxfm,
            bool teammate, float scale, float speed, int snap,
            float basetime);

} /* namespace md2 */
} /* namespace q */

