PS_IN vec3 fs_nor;
PS_IN vec3 fs_pos;
void main() {
  vec3 p = fs_pos*3.0;
  float c = snoise(p);
  float dx = snoise(p+vec3(0.01,0.0,0.0));
  float dy = snoise(p+vec3(0.0,0.01,0.0));
  float dz = snoise(p+vec3(0.0,0.0,0.01));
  vec3 dn = normalize(vec3(c-dx, c-dy, c-dz));
  vec3 n = 0.5*normalize(fs_nor + dn/10.0)+0.5;
  SWITCH_WEBGL(gl_FragData[0], rt_col) = vec4(1.0);
  SWITCH_WEBGL(gl_FragData[1], rt_nor) = vec4(n, 1.0);
}

