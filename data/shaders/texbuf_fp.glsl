void main() {
  ivec2 ipos = ivec2(gl_FragCoord.xy);
  rt_col = texelFetch(u_texbuf, (u_width-1-ipos.x)+ipos.y*u_width);
}

