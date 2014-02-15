VS_OUT vec2 fs_tex;
VS_OUT vec3 fs_nor;
void main() {
  fs_tex = vs_tex;
  fs_nor = u_nortransform*mix(vs_nor0,vs_nor1,u_delta);
  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}

