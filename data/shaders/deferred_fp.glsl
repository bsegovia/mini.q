//##const char deferred_fp[] = {
void main() {
  vec2 uv = gl_FragCoord.xy;
  vec3 nor = normalize(2.0*texture2DRect(u_nortex, uv).xyz-1.0);
  float depth = texture2DRect(u_depthtex, uv).r;
  vec4 outcol;
  if (depth != 1.0) {
    vec4 posw = u_invmvp * vec4(uv, depth, 1.0);
    vec3 pos = posw.xyz / posw.w;
    vec3 ldir = u_lightpos-pos;
    float llen2 = dot(ldir,ldir);
    outcol.xyz = vec3(2000.0) * max(dot(nor,ldir),0.0) / (llen2*llen2);
    outcol.w = 1.0;
  } else {
    vec4 rdh = u_dirinvmvp * vec4(uv, 0.0, 1.0);
    vec3 rd = normalize(rdh.xyz/rdh.w);
    outcol = vec4(getsky(rd, u_sundir), 1.0);
  }
  SWITCH_WEBGL(gl_FragColor, rt_col) = outcol;
}
//##};

