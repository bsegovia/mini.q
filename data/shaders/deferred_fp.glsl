//##const char deferred_fp[] = {
uniform sampler2DRect u_nortex;
uniform vec2 u_subbufferdim;
uniform vec2 u_rcpsubbufferdim;
IF_NOT_WEBGL(out vec4 rt_c);

void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec2 bufindex = uv * u_rcpsubbufferdim;
  vec2 pixindex = mod(uv, u_subbufferdim);
  vec2 splituv = SPLITNUM * pixindex + bufindex;
  vec4 nor = texture2DRect(u_nortex, splituv);
  SWITCH_WEBGL(gl_FragColor = abs(nor), rt_c = abs(nor));
}
//##};

