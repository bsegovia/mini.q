#if USE_DIFFUSETEX
PS_IN vec2 fs_tex;
#endif
#if USE_COL
PS_IN vec4 fs_col;
#endif
IF_NOT_WEBGL(out vec4 rt_col);

void main() {
  vec4 col;
#if USE_COL
  vec4 incol = fs_col;
#else
  vec4 incol = vec4(1.0);
#endif
#if USE_DIFFUSETEX
  col = texture2D(u_diffuse, fs_tex);
  col *= incol;
#else
  col = incol;
#endif
  SWITCH_WEBGL(gl_FragColor, rt_col) = col;
}

