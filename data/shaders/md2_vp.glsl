VS_OUT vec2 fs_tex;
VS_OUT vec3 fs_nor;
void main() {
  fs_nor = vs_nor;
  fs_tex = vs_tex;
  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}

