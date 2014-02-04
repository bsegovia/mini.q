//##const char debugunsplit_fp[] = {
uniform sampler2DRect u_nortex;
IF_NOT_WEBGL(out vec4 rt_c);

void main() {
  vec2 uv = gl_FragCoord.xy;
  vec2 bufindex = uv / vec2(320.0,256.0);
  vec2 pixindex = mod(uv, vec2(320.0,256.0));
  vec2 interleaveduv = 4.0 * pixindex + bufindex;
  vec4 nor = texture2DRect(u_nortex, uv);
  //vec4 nor = texture2DRect(u_nortex, interleaveduv);
  SWITCH_WEBGL(gl_FragColor = abs(nor), rt_c = abs(nor));
}
//##};

