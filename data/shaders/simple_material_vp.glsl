//##const char simple_material_vp[] = {
VS_OUT vec3 fs_pos;
VS_OUT vec3 fs_nor;
void main() {
  fs_nor = vs_nor;
  fs_pos = vs_pos;
  gl_Position = u_mvp*vec4(vs_pos,1.0);
}
//##};

