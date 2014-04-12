void main() {
  ivec2 ipos = ivec2(gl_FragCoord.xy);
  rt_col = texelFetch(u_texbuf, ipos.x+ipos.y*u_width);
}

