//##const char simple_material_fp[] = {
PS_IN vec3 fs_nor;
void main() {
  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(abs(fs_nor.xyz), 1.0);
}
//##};

