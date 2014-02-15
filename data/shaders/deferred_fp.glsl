void main() {
  vec2 uv = gl_FragCoord.xy;
  vec3 nor = normalize(2.0*texture2DRect(u_nortex, uv).xyz-1.0);
  float depth = texture2DRect(u_depthtex, uv).r;
  vec4 outcol;
  if (depth != 1.0) {
    vec4 posw = u_invmvp * vec4(uv, depth, 1.0);
    vec3 pos = posw.xyz / posw.w;
    vec4 diffuse = texture2DRect(u_diffusetex, uv);
    outcol = diffuse*vec4(shade(pos, nor), 1.0);
  } else {
    vec4 rdh = u_dirinvmvp * vec4(uv, 0.0, 1.0);
    vec3 rd = normalize(rdh.xyz/rdh.w);
    outcol = vec4(getsky(rd, u_sundir), 1.0);
  }
  SWITCH_WEBGL(gl_FragColor, rt_col) = outcol;
}

