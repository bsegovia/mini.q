//###if DEBUG_UNSPLIT
//##const char debugunsplit_fp[] = {
#define SAMPLE(NUM, X, Y) \
  vec2 uv##NUM = uv+vec2(X,Y);\
  vec2 bufindex##NUM = mod(uv##NUM, SPLITNUM);\
  vec2 pixindex##NUM = uv##NUM / vec2(SPLITNUM);\
  vec2 unsplituv##NUM = pixindex##NUM + bufindex##NUM * u_subbufferdim;\
  vec3 col##NUM = texture2DRect(u_lighttex, unsplituv##NUM).xyz;\
  vec2 nor##NUM = texture2DRect(u_nortex, uv##NUM).xy;\
  bool ok##NUM = dot(nor##NUM, nor) > 0.8;\
  col += ok##NUM ? col##NUM : vec3(0.0);\
  num += ok##NUM ? 1.0 : 0.0;

void main() {
  vec2 uv = floor(gl_FragCoord.xy);
  vec2 bufindex = mod(uv, SPLITNUM);
  vec2 pixindex = uv / vec2(SPLITNUM);
  vec2 unsplituv = pixindex + bufindex * u_subbufferdim;
  vec2 nor = texture2DRect(u_nortex, uv).xy;
  vec3 col = vec3(0.0);
  float num = 0.0;
  SAMPLE(0,  -1.0, +1.0);
  SAMPLE(1,  -1.0,  0.0);
  SAMPLE(2,  -1.0, -1.0);
  SAMPLE(3,  -1.0, -2.0);
  SAMPLE(4,   0.0, +1.0);
  SAMPLE(5,   0.0,  0.0);
  SAMPLE(6,   0.0, -1.0);
  SAMPLE(7,   0.0, -2.0);
  SAMPLE(8,  +1.0, +1.0);
  SAMPLE(9,  +1.0,  0.0);
  SAMPLE(10, +1.0, -1.0);
  SAMPLE(11, +1.0, -2.0);
  SAMPLE(12, +2.0, +1.0);
  SAMPLE(13, +2.0,  0.0);
  SAMPLE(14, +2.0, -1.0);
  SAMPLE(15, +2.0, -2.0);
  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(col, 1.0);
}
//##};
//###endif

