//## const char fixed_fp[] = {
#if USE_DIFFUSETEX
uniform sampler2D u_diffuse;
PS_IN vec2 fs_tex;
#endif
#if USE_FOG
uniform vec4 u_fogcolor;
uniform vec2 u_fogstartend;
PS_IN float fs_fogz;
#endif
PS_IN vec4 fs_col;
IF_NOT_WEBGL(out vec4 rt_c);

void main() {
  vec4 col;
#if USE_DIFFUSETEX
  col = texture2D(u_diffuse, fs_tex);
  col *= fs_col;
#else
  col = fs_col;
#endif
#if USE_FOG
  float factor = clamp((-fs_fogz-u_fogstartend.x)*u_fogstartend.y,0.0,1.0);
  col.xyz = mix(col.xyz,u_fogcolor.xyz,factor);
#endif
  SWITCH_WEBGL(gl_FragColor = col;, rt_c = col;)
}
//## };

