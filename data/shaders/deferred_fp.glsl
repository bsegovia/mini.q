//##const char deferred_fp[] = {
void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec3 nor = normalize(2.0*texture2DRect(u_nortex, uv).xyz-1.0);
  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(nor,1.0);
}
//##};

