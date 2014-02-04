//##const char debugunsplit_fp[] = {
uniform sampler2DRect u_nortex;
uniform vec2 u_subbufferdim;
uniform vec2 u_rcpsubbufferdim;
IF_NOT_WEBGL(out vec4 rt_c);

void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec2 bufindex = mod(uv, SPLITNUM);
  vec2 pixindex = uv / vec2(SPLITNUM);
  vec2 unsplituv = pixindex + bufindex * u_subbufferdim;
  vec4 nor = texture2DRect(u_nortex, unsplituv);
  SWITCH_WEBGL(gl_FragColor = abs(nor), rt_c = abs(nor));
}
//##};

