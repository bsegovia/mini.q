//##const char font_fp[] = {
PS_IN vec2 fs_tex;

#define RSQ2 0.7071078

void distseg(inout float dist, vec2 start, vec2 end, vec2 nor, vec2 pos) {
  bool inside =  (dot(pos-start,end-start)>=0.0 && dot(end-pos,end-start)>=0.0);
  dist = inside ? min(dist, abs(dot(pos-start, nor))) : dist;
}

void main() {
  vec2 uv = fs_tex;
  vec2 c = vec2(0.5);
  vec2 pos = fract(uv * u_fontwh);
  vec2 duv = vec2(1.0) / u_fontwh;
  float s  = texture2D(u_diffuse, uv).r;
  float l  = texture2D(u_diffuse, uv+vec2(-duv.x,0.0)).r;
  float r  = texture2D(u_diffuse, uv+vec2(+duv.x,0.0)).r;
  float t  = texture2D(u_diffuse, uv+vec2(0.0, duv.y)).r;
  float b  = texture2D(u_diffuse, uv+vec2(0.0,-duv.y)).r;
  float tl = texture2D(u_diffuse, uv+vec2(-duv.x, duv.y)).r;
  float tr = texture2D(u_diffuse, uv+vec2( duv.x, duv.y)).r;
  float bl = texture2D(u_diffuse, uv+vec2(-duv.x,-duv.y)).r;
  float br = texture2D(u_diffuse, uv+vec2( duv.x,-duv.y)).r;
  float dist = 1.0;
  if (s != 0.0) {
    dist = distance(pos,c);
    if (l!=0.0) distseg(dist, c, vec2(-0.5,0.5), vec2(0.0,1.0), pos);
    if (r!=0.0) distseg(dist, c, vec2(+1.5,0.5), vec2(0.0,1.0), pos);
    if (t!=0.0) distseg(dist, c, vec2(0.5,+1.5), vec2(1.0,0.0), pos);
    if (b!=0.0) distseg(dist, c, vec2(0.5,-0.5), vec2(1.0,0.0), pos);
    if (l==0.0&&t==0.0&&tl!=0.0) distseg(dist, c, vec2(-0.5,+1.5), vec2(+RSQ2,RSQ2), pos);
    if (r==0.0&&t==0.0&&tr!=0.0) distseg(dist, c, vec2(+1.5,+1.5), vec2(-RSQ2,RSQ2), pos);
    if (l==0.0&&b==0.0&&bl!=0.0) distseg(dist, c, vec2(-0.5,-0.5), vec2(-RSQ2,RSQ2), pos);
    if (r==0.0&&b==0.0&&br!=0.0) distseg(dist, c, vec2(+1.5,-0.5), vec2(+RSQ2,RSQ2), pos);
  } else {
    if (t!=0.0&&r!=0.0&&tr==0.0) distseg(dist, vec2(0.5,+1.5), vec2(+1.5,0.5), vec2(+RSQ2,+RSQ2), pos);
    if (t!=0.0&&l!=0.0&&tl==0.0) distseg(dist, vec2(0.5,+1.5), vec2(-0.5,0.5), vec2(-RSQ2,+RSQ2), pos);
    if (b!=0.0&&r!=0.0&&br==0.0) distseg(dist, vec2(0.5,-0.5), vec2(+1.5,0.5), vec2(-RSQ2,+RSQ2), pos);
    if (b!=0.0&&l!=0.0&&bl==0.0) distseg(dist, vec2(0.5,-0.5), vec2(-0.5,0.5), vec2(+RSQ2,+RSQ2), pos);
  }
  float no = u_font_thickness-u_outline_width;
  float o = u_font_thickness;
  vec4 col = vec4(1.0-smoothstep(no-0.1, no, dist));
  col += u_outline_color * (1.0-smoothstep(o-0.1, o, dist));
  SWITCH_WEBGL(gl_FragColor, rt_col) = col;
}
//##};

