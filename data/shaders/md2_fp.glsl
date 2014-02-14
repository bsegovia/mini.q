PS_IN vec2 fs_tex;
PS_IN vec3 fs_nor;
void main() {
  SWITCH_WEBGL(gl_FragData[0], rt_nor) = vec4(normalize(fs_nor), 1.0);
  SWITCH_WEBGL(gl_FragData[1], rt_col) = texture2D(u_diffuse, fs_tex);
}

