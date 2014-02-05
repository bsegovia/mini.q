//##const char deferred_fp[] = {
uniform sampler2DRect u_nortex;
uniform sampler2DRect u_depthtex;
uniform vec2 u_subbufferdim;
uniform vec2 u_rcpsubbufferdim;
uniform mat4 u_invmvp;
IF_NOT_WEBGL(out vec4 rt_c);

const vec3 lightdir = normalize(vec3(1.0,-1.0,0.0));

void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec2 bufindex = uv * u_rcpsubbufferdim;
  vec2 pixindex = mod(uv, u_subbufferdim);
  vec2 splituv = SPLITNUM * pixindex + bufindex;

  vec3 nor = 2.0 * texture2DRect(u_nortex, splituv).xyz - 1.0;
  float depth = texture2DRect(u_depthtex, splituv).r;

  vec4 posw = u_invmvp * vec4(splituv, depth, 1.0);
  vec3 pos = posw.xyz / posw.w;
  float col = max(dot(nor,lightdir),0.0);
  //SWITCH_WEBGL(gl_FragColor, rt_c) = vec4(col,col,col,1.0);
  //SWITCH_WEBGL(gl_FragColor, rt_c) = vec4(depth,depth,depth,1.0);
  SWITCH_WEBGL(gl_FragColor, rt_c) = vec4(pos/30.0,1.0);
//  SWITCH_WEBGL(gl_FragColor = abs(2.0*nor-1.0), rt_c = abs(2.0*nor-1.0));
}
//##};

