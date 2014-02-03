//##const char fixed_vp[] = {
uniform mat4 u_mvp;
#if USE_KEYFRAME
uniform float u_delta;
VS_IN vec3 vs_pos0, vs_pos1;
#else
VS_IN vec3 vs_pos;
#endif
#if USE_COL
VS_IN vec4 vs_col;
VS_OUT vec4 fs_col;
#endif
#if USE_DIFFUSETEX
VS_IN vec2 vs_tex;
VS_OUT vec2 fs_tex;
#endif
void main() {
#if USE_DIFFUSETEX
  fs_tex = vs_tex;
#endif
#if USE_COL
  fs_col = vs_col;
#endif
#if USE_KEYFRAME
  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);
#endif
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}
//##};

