//##// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0
//##// Unported License.
//##// From rolling hills by Dave Hoskins (https://www.shadertoy.com/view/Xsf3zX)

const float PI = 3.141592653, PI2 = 2.0*PI;
const vec3 SUN_COLOR = vec3(1.0, 0.75, 0.6);

vec3 getsky(vec3 rd, bool withsun, vec3 sunlight, vec3 suncolour) {
  float sunamount = withsun ? max(dot(rd, sunlight), 0.0 ) : 0.0;
  float factor = pow(1.0-max(rd.y,0.0),6.0);
  vec3 sun = suncolour * sunamount * sunamount * 0.25
           + SUN_COLOR * min(pow(sunamount, 800.0)*1.5, 0.3);
  return clamp(mix(vec3(0.1,0.2,0.3),vec3(0.32,0.32,0.32), factor)+sun,0.0,1.0);
}

vec3 getsky(vec3 rd, vec3 sunlight) {
  // X = north, Y = top
  vec3 suncolour = SUN_COLOR;
  if (sunlight.y < 0.12)
    suncolour = mix(vec3(2.0,0.4,0.2), SUN_COLOR, smoothstep(0.0,1.0,sunlight.y*8.0));
  vec3 col = sqrt(getsky(rd, true, sunlight, suncolour));
  return col + suncolour*mix(0.0, 0.051, sunlight.y<0.05 ? 1.0-sunlight.y*20.0 : 0.0);
}

