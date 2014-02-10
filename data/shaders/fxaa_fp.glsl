//##const char fxaa_fp[] = {
void main() {
  rt_col = FxaaPixelShader(gl_FragCoord.xy*u_rcptexsize, u_tex, u_rcptexsize, 0.75, 0.125, 0.0);
}
//##};

