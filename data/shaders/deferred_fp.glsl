//##const char deferred_fp[] = {
uniform sampler2DRect u_nortex;
uniform sampler2DRect u_depthtex;
uniform vec2 u_subbufferdim;
uniform vec2 u_rcpsubbufferdim;
uniform mat4 u_invmvp;
IF_NOT_WEBGL(out vec4 rt_c);

uniform vec3 u_lightpos;// = vec3(4.f,4.f,4.f);
uniform vec3 u_lightpow;// = vec3(100.f,0.f,0.f);

void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec2 bufindex = uv * u_rcpsubbufferdim;
  vec2 pixindex = mod(uv, u_subbufferdim);
  vec2 splituv = SPLITNUM * pixindex + bufindex;
  vec3 nor = normalize(2.0*texture2DRect(u_nortex, splituv).xyz-1.0);
  float depth = texture2DRect(u_depthtex, splituv).r;
  vec4 posw = u_invmvp * vec4(splituv, depth, 1.0);
  vec3 pos = posw.xyz / posw.w;

  vec3 ldir = u_lightpos-pos;
  float llen2 = dot(ldir,ldir);
  vec3 lit = u_lightpow * max(dot(nor,ldir),0.0) / (llen2*llen2);
  SWITCH_WEBGL(gl_FragColor, rt_c) = vec4(lit,1.0);
  //SWITCH_WEBGL(gl_FragColor, rt_c) = vec4(nor,1.0);
}
//##};

