#if 0
const float PI=3.14159265358979323846;
float speed=iGlobalTime*0.2975;
float ground_x=1.0-0.325*sin(PI*speed*0.25);
float ground_y=1.0;
float ground_z=0.5;

vec2 rotate(vec2 k,float t) {
  return vec2(cos(t)*k.x-sin(t)*k.y,sin(t)*k.x+cos(t)*k.y);
}

float draw_scene(vec3 p) {
  float tunnel_m=0.125*cos(PI*p.z*1.0+speed*4.0-PI);
  float tunnel1_p=2.0;
  float tunnel1_w=tunnel1_p*0.225;
  float tunnel1=length(mod(p.xy,tunnel1_p)-tunnel1_p*0.5)-tunnel1_w;  // tunnel1
  float tunnel2_p=2.0;
  float tunnel2_w=tunnel2_p*0.2125+tunnel2_p*0.0125*cos(PI*p.y*8.0)+tunnel2_p*0.0125*cos(PI*p.z*8.0);
  float tunnel2=length(mod(p.xy,tunnel2_p)-tunnel2_p*0.5)-tunnel2_w;  // tunnel2
  float hole1_p=1.0;
  float hole1_w=hole1_p*0.5;
  float hole1=length(mod(p.xz,hole1_p).xy-hole1_p*0.5)-hole1_w;  // hole1
  float hole2_p=0.25;
  float hole2_w=hole2_p*0.375;
  float hole2=length(mod(p.yz,hole2_p).xy-hole2_p*0.5)-hole2_w;  // hole2
  float hole3_p=0.5;
  float hole3_w=hole3_p*0.25+0.125*sin(PI*p.z*2.0);
  float hole3=length(mod(p.xy,hole3_p).xy-hole3_p*0.5)-hole3_w;  // hole3
  float tube_m=0.075*sin(PI*p.z*1.0);
  float tube_p=0.5+tube_m;
  float tube_w=tube_p*0.025+0.00125*cos(PI*p.z*128.0);
  float tube=length(mod(p.xy,tube_p)-tube_p*0.5)-tube_w;      // tube
  float bubble_p=0.05;
  float bubble_w=bubble_p*0.5+0.025*cos(PI*p.z*2.0);
  float bubble=length(mod(p.yz,bubble_p)-bubble_p*0.5)-bubble_w;  // bubble
  return max(min(min(-tunnel1,mix(tunnel2,-bubble,0.375)),max(min(-hole1,hole2),-hole3)),-tube);
}

vec4 entry(void) {
  vec2 position=(gl_FragCoord.xy/iResolution.xy);
  vec2 p=-1.0+2.0*position;
  vec3 dir=normalize(vec3(p*vec2(1.77,1.0),1.0));    // screen ratio (x,y) fov (z)
  dir.zx=rotate(dir.zx,-PI*speed*0.25);        // rotation y
  dir.xy=rotate(dir.xy,-speed*0.5);          // rotation z
  vec3 ray=vec3(ground_x,ground_y,ground_z-speed*2.5);
  float t=0.0;
  const int ray_n=96;
  for(int i=0;i<ray_n;i++) {
    float k=draw_scene(ray+dir*t);
    t+=k*0.75;
  }
  vec3 hit=ray+dir*t;
  vec2 h=vec2(-0.0025,0.002); // light
  vec3 n=normalize(vec3(draw_scene(hit+h.xyx),draw_scene(hit+h.yxy),draw_scene(hit+h.yyx)));
  float c=(n.x+n.y+n.z)*0.35;
  vec3 color=vec3(c,c,c)+t*0.0625;
  return vec4(vec3(c-t*0.0375+p.y*0.05,c-t*0.025-p.y*0.0625,c+t*0.025-p.y*0.025)+color*color,1.0);
}
#else
vec4 diffuse(vec2 p) {
  vec2 sq = abs(0.5-fract(5.0*p));
  vec4 black = mix(vec4(0.3), vec4(0.4,0.3,0.3,0.2), vec4(snoise(200.0*p)));
  vec4 whitedirt = mix(vec4(0.7), vec4(0.6), vec4(snoise(200.0*p)));
  whitedirt += mix(vec4(0.1), vec4(0.15), vec4(snoise(50.0*p)));
  whitedirt += mix(vec4(0.05), vec4(0.08), vec4(snoise(20.0*p)));
  vec2 s = smoothstep(vec2(0.015), vec2(0.020 + 0.005*snoise(20.0+20.0*p)), sq);
  vec4 col = mix(black, whitedirt, s.x*s.y);
  return col;
}

float height(vec2 p) {
  vec2 sq = abs(0.5-fract(5.0*p));
  float blackh = mix(0.44, 0.45, snoise(100.0*p));
  float whitedirth = mix(0.48, 0.49, snoise(100.0*p));
  whitedirth += mix(0.48, 0.49, snoise(25.0*p));
  whitedirth += mix(0.48, 0.49, snoise(10.0*p));
  vec2 s = smoothstep(vec2(0.015), vec2(0.030 + 0.005*snoise(20.0+20.0*p)), sq);
  float h = mix(blackh, whitedirth, s.x*s.y);
  return h;
}

vec4 entry(void) {
  vec3 ldir = normalize(vec3(cos(iGlobalTime)*0.5,0.5,0.5));
  vec2 p = gl_FragCoord.xy/iResolution.xy;
  vec4 col = diffuse(p);
  float h  = height(p);
  float hx = height((gl_FragCoord.xy+vec2(0.01,0.0))/iResolution.xy);
  float hy = height((gl_FragCoord.xy+vec2(0.0,0.01))/iResolution.xy);
  vec2 n = 0.05f*normalize(vec2(h-hx,h-hy));
  vec3 gn = normalize(vec3(n.x,1.0,n.y));
  col *= 1.3;
  return col * max(dot(gn,ldir),0.0);
//  return vec4(abs(n), 1.0, 1.0); //col;
  //return col;
}
#endif

