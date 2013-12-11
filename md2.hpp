/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - md2.cpp -> exposes quake md2 model routines
 -------------------------------------------------------------------------*/
#include "mini.q.hpp"

namespace q {
namespace md2 {

void render(const char *name, int frame, int range,
            float rad, const vec3f &o, const vec3f &ypr,
            bool teammate, float scale, float speed, int snap,
            float basetime);

} /* namespace md2 */
} /* namespace q */

