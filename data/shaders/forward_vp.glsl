//##const char forward_vp[] = {
VS_OUT vec3 fs_nor;
void entry(inout vec3 fs_nor, in vec3 vs_nor) {
  fs_nor = vs_nor;
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}
void main() {entry(fs_nor, vs_nor);}
//##};

