
//## const char font_fp[] = {

uniform sampler2D u_diffuse;
PS_IN vec2 fs_tex;
IF_NOT_WEBGL(out vec4 rt_c);

#define FONTW 128
#define FONTH 96
#define RSQ2 0.7071078

void distseg(inout float dist, vec2 start, vec2 end, vec2 nor, vec2 pos) {
  bool inside =  (dot(pos-start,end-start)>=0.0 && dot(end-pos,end-start)>=0.0);
  dist = inside ? min(dist, abs(dot(pos-start, nor))) : dist;
}

void main() {
  vec2 uv = fs_tex;
  vec2 c = vec2(0.5);
  vec2 pos = fract(uv * vec2(FONTW, FONTH));
  float du = 1.0 / float(FONTW);
  float dv = 1.0 / float(FONTH);
  float s  = texture2D(u_diffuse, uv).r;
  float l  = texture2D(u_diffuse, uv+vec2(-du,0.0)).r;
  float r  = texture2D(u_diffuse, uv+vec2(+du,0.0)).r;
  float t  = texture2D(u_diffuse, uv+vec2(0.0, dv)).r;
  float b  = texture2D(u_diffuse, uv+vec2(0.0,-dv)).r;
  float tl = texture2D(u_diffuse, uv+vec2(-du, dv)).r;
  float tr = texture2D(u_diffuse, uv+vec2( du, dv)).r;
  float bl = texture2D(u_diffuse, uv+vec2(-du,-dv)).r;
  float br = texture2D(u_diffuse, uv+vec2( du,-dv)).r;
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
  vec4 col = dist < 0.4 ? vec4(1.0) : vec4(0.0);
  SWITCH_WEBGL(gl_FragColor = col, rt_c = col);
}
//## };

