//##const char forward_vp[] = {
#if 0
uniform mat4 u_mvp;
VS_IN vec3 vs_pos;
VS_IN vec3 vs_nor;
#endif
VS_OUT vec3 fs_nor;

void main() {
  fs_nor = vs_nor;
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}
//##};

