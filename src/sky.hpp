/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sky.cpp -> get sun position at any given time
 -------------------------------------------------------------------------*/

#include "base/math.hpp"
namespace q {
namespace sky {

float julianday2000(int yr, int mn, int day, int hr, int m, int s);
float julianday2000(float unixtimems);
vec3f sunvector(float jd, float latitude, float longitude);

} /* namespace sky */
} /* namespace q */

