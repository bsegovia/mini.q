//## const char font_fp[] = {
uniform sampler2D u_diffuse;
PS_IN vec2 fs_tex;
IF_NOT_WEBGL(out vec4 rt_c);

void main() {
  vec4 col = smoothstep(0.35,0.45,texture2D(u_diffuse, fs_tex));
  SWITCH_WEBGL(gl_FragColor = col;, rt_c = col;)
}
//## };

