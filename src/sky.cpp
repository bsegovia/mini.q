/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - sky.cpp -> computes sun position at a given time
 -------------------------------------------------------------------------*/

// Based on LOW-Precision formulae for planetary positions by T.C. Van Flandern
// and H-F. Pulkkinen
// http://articles.adsabs.harvard.edu/cgi-bin/nph-iarticle_query?1979ApJS...41..391V&defaultprint=YES&filetype=.pdf
// and from shadertoy shader:
// https://www.shadertoy.com/view/XsS3Dm

#include "math.hpp"
#include "sky.hpp"

namespace q {
namespace sky {

static const float PI2 = 2.f*float(pi);
// Astronomical Unit = Mean distance of the earth from the sun - Km
// static const float AU = 149597870.61;
static const float TORAD = float(pi)/180.f;;

float julianday2000(int yr, int mn, int day, int hr, int m, int s) {
  const int im = (mn-14)/12;
  const int ijulian = day - 32075 + 1461*(yr+4800+im)/4
                    + 367*(mn-2-im*12)/12 - 3*((yr+4900+im)/100)/4;
  const auto f = float(ijulian)-2451545.f;
  return f - 0.5f + float(hr)/24.f + float(m)/1440.f + float(s)/86400.f;
}

float julianday2000(float unixtimems) {
  return (unixtimems/86400.f)-10957.5f;
}

static vec2f sunattime(float julianday2000, float latitude, float longitude) {
  //= jd - 2451545
  // nb julian days since 01/01/2000 (1 January 2000 = 2451545 Julian Days)
  const auto t = julianday2000;
  // nb julian centuries since 2000
  const auto t0 = t/36525.f; 
  // nb julian centuries since 1900
  const auto t1 = t0+1.f;
  // mean longitude of sun
  const auto Ls = fract(.779072f+.00273790931f*t)*PI2;
  // mean anomaly of sun
  const auto Ms = fract(.993126f+.0027377785f*t)*PI2;
  // Greenwich Mean Sidereal Time
  const auto GMST = 280.46061837f + 360.98564736629f*t + (0.000387933f - t0/38710000.f)*t0*t0;

  // position of sun
  const auto v = (.39785f-.00021f*t1)*sin(Ls)-.01f*sin(Ls-Ms)+.00333f*sin(Ls+Ms);
  const auto u = 1.f-.03349f*cos(Ms)-.00014f*cos(2.f*Ls)+.00008f*cos(Ls);
  const auto w = -.0001f-.04129f * sin(2.f*Ls)+(.03211f-.00008f*t1)*sin(Ms)
               + .00104f*sin(2.f*Ls-Ms)-.00035f*sin(2.f*Ls+Ms);
  // calcul right ascention
  const auto zs = w / sqrt(u-v*v);
  const auto rightAscention = Ls + atan(zs/sqrt(1.-zs*zs));
  // calcul declination
  const auto zzs = v / sqrt(u);
  const auto declination = atan(zzs/sqrt(1.-zzs*zzs));
  // position relative to geographic location
  const auto sin_dec = sin(declination),   cos_dec = cos(declination);
  const auto sin_lat = sin(TORAD*latitude),cos_lat = cos(TORAD*latitude);
  auto lmst = mod((GMST + longitude)/15.f, 24.f);
  if (lmst<0.f) lmst += 24.f;
  lmst = TORAD*lmst*15.f;
  const auto ha = lmst - rightAscention;
  const auto elevation = asin(sin_lat * sin_dec + cos_lat * cos_dec * cos(ha));
  const auto azimuth   = acos((sin_dec - (sin_lat*sin(elevation))) / (cos_lat*cos(elevation)));
  return vec2f(sin(ha)>0.f? azimuth:PI2-azimuth, elevation);
}

vec3f sunvector(float jd, float latitude, float longitude) {
  vec2f ae = sunattime(jd, latitude, longitude);
  // X = north, Y = top
  return normalize(vec3f(-cos(ae.x)*cos(ae.y), sin(ae.y), sin(ae.x)*cos(ae.y)));
}
} /* namespace sky */
} /* namespace q */

