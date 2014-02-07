/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> stores shaders (do not modify)
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {
#if DEBUG_UNSPLIT
const char debugunsplit_fp[] = {
"uniform sampler2DRect u_lighttex;\n"
"uniform sampler2DRect u_nortex;\n"
"uniform vec2 u_subbufferdim;\n"
"uniform vec2 u_rcpsubbufferdim;\n"
"IF_NOT_WEBGL(out vec4 rt_col);\n"

"#define SAMPLE(NUM, X, Y) \\n"
"  vec2 uv##NUM = uv+vec2(X,Y);\\n"
"  vec2 bufindex##NUM = mod(uv##NUM, SPLITNUM);\\n"
"  vec2 pixindex##NUM = uv##NUM / vec2(SPLITNUM);\\n"
"  vec2 unsplituv##NUM = pixindex##NUM + bufindex##NUM * u_subbufferdim;\\n"
"  vec3 col##NUM = texture2DRect(u_lighttex, unsplituv##NUM).xyz;\\n"
"  vec2 nor##NUM = texture2DRect(u_nortex, uv##NUM).xy;\\n"
"  bool ok##NUM = dot(nor##NUM, nor) > 0.8;\\n"
"  col += ok##NUM ? col##NUM : vec3(0.0);\\n"
"  num += ok##NUM ? 1.0 : 0.0;\n"

"void main() {\n"
"  vec2 uv = floor(gl_FragCoord.xy);\n"
"  vec2 bufindex = mod(uv, SPLITNUM);\n"
"  vec2 pixindex = uv / vec2(SPLITNUM);\n"
"  vec2 unsplituv = pixindex + bufindex * u_subbufferdim;\n"
"  vec2 nor = texture2DRect(u_nortex, uv).xy;\n"
"  vec3 col = vec3(0.0);\n"
"  float num = 0.0;\n"
"  SAMPLE(0,  -1.0, +1.0);\n"
"  SAMPLE(1,  -1.0,  0.0);\n"
"  SAMPLE(2,  -1.0, -1.0);\n"
"  SAMPLE(3,  -1.0, -2.0);\n"
"  SAMPLE(4,   0.0, +1.0);\n"
"  SAMPLE(5,   0.0,  0.0);\n"
"  SAMPLE(6,   0.0, -1.0);\n"
"  SAMPLE(7,   0.0, -2.0);\n"
"  SAMPLE(8,  +1.0, +1.0);\n"
"  SAMPLE(9,  +1.0,  0.0);\n"
"  SAMPLE(10, +1.0, -1.0);\n"
"  SAMPLE(11, +1.0, -2.0);\n"
"  SAMPLE(12, +2.0, +1.0);\n"
"  SAMPLE(13, +2.0,  0.0);\n"
"  SAMPLE(14, +2.0, -1.0);\n"
"  SAMPLE(15, +2.0, -2.0);\n"
"  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(col, 1.0);\n"
"}\n"
};
#endif

#if DEBUG_UNSPLIT
const char debugunsplit_vp[] = {
"uniform mat4 u_mvp;\n"
"VS_IN vec2 vs_pos;\n"
"void main() {gl_Position = u_mvp*vec4(vs_pos,1.0,1.0);}\n"
};
#endif

const char deferred_fp[] = {
"uniform sampler2DRect u_nortex;\n"
"uniform sampler2DRect u_depthtex;\n"
"uniform vec2 u_subbufferdim;\n"
"uniform vec2 u_rcpsubbufferdim;\n"
"uniform mat4 u_invmvp;\n"
"uniform vec3 u_lightpos;\n"
"uniform vec3 u_lightpow;\n"
"IF_NOT_WEBGL(out vec4 rt_col);\n"

"void main() {\n"
"  vec2 uv = floor(gl_FragCoord.xy);\n"
"  vec2 bufindex = uv * u_rcpsubbufferdim;\n"
"  vec2 pixindex = mod(uv, u_subbufferdim);\n"
"  vec2 splituv = SPLITNUM * pixindex + bufindex;\n"
"  vec3 nor = normalize(2.0*texture2DRect(u_nortex, splituv).xyz-1.0);\n"
"  float depth = texture2DRect(u_depthtex, splituv).r;\n"
"  vec4 posw = u_invmvp * vec4(splituv, depth, 1.0);\n"
"  vec3 pos = posw.xyz / posw.w;\n"
"  vec3 ldir = u_lightpos-pos;\n"
"  float llen2 = dot(ldir,ldir);\n"
"  vec3 lit = u_lightpow * max(dot(nor,ldir),0.0) / (llen2*llen2);\n"
"  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(lit,1.0);\n"
"}\n"
};

const char deferred_vp[] = {
"uniform mat4 u_mvp;\n"
"VS_IN vec2 vs_pos;\n"
"void main() {gl_Position = u_mvp*vec4(vs_pos,1.0,1.0);}\n"
};

const char fixed_fp[] = {
"#if USE_DIFFUSETEX\n"
"uniform sampler2D u_diffuse;\n"
"PS_IN vec2 fs_tex;\n"
"#endif\n"
"#if USE_COL\n"
"PS_IN vec4 fs_col;\n"
"#endif\n"
"IF_NOT_WEBGL(out vec4 rt_col);\n"

"void main() {\n"
"  vec4 col;\n"
"#if USE_COL\n"
"  vec4 incol = fs_col;\n"
"#else\n"
"  vec4 incol = vec4(1.0);\n"
"#endif\n"
"#if USE_DIFFUSETEX\n"
"  col = texture2D(u_diffuse, fs_tex);\n"
"  col *= incol;\n"
"#else\n"
"  col = incol;\n"
"#endif\n"
"  SWITCH_WEBGL(gl_FragColor, rt_col) = col;\n"
"}\n"
};

const char fixed_vp[] = {
"uniform mat4 u_mvp;\n"
"#if USE_KEYFRAME\n"
"uniform float u_delta;\n"
"VS_IN vec3 vs_pos0, vs_pos1;\n"
"#else\n"
"VS_IN vec3 vs_pos;\n"
"#endif\n"
"#if USE_COL\n"
"VS_IN vec4 vs_col;\n"
"VS_OUT vec4 fs_col;\n"
"#endif\n"
"#if USE_DIFFUSETEX\n"
"VS_IN vec2 vs_tex;\n"
"VS_OUT vec2 fs_tex;\n"
"#endif\n"
"void main() {\n"
"#if USE_DIFFUSETEX\n"
"  fs_tex = vs_tex;\n"
"#endif\n"
"#if USE_COL\n"
"  fs_col = vs_col;\n"
"#endif\n"
"#if USE_KEYFRAME\n"
"  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);\n"
"#endif\n"
"  gl_Position = u_mvp*vec4(vs_pos,1.0);\n"
"}\n"
};

const char font_fp[] = {
"uniform sampler2D u_diffuse;\n"
"uniform vec2 u_fontwh;\n"
"uniform float u_font_thickness;\n"
"uniform float u_outline_width;\n"
"uniform vec4 u_outline_color;\n"
"PS_IN vec2 fs_tex;\n"
"IF_NOT_WEBGL(out vec4 rt_col);\n"

"#define RSQ2 0.7071078\n"

"void distseg(inout float dist, vec2 start, vec2 end, vec2 nor, vec2 pos) {\n"
"  bool inside =  (dot(pos-start,end-start)>=0.0 && dot(end-pos,end-start)>=0.0);\n"
"  dist = inside ? min(dist, abs(dot(pos-start, nor))) : dist;\n"
"}\n"

"void main() {\n"
"  vec2 uv = fs_tex;\n"
"  vec2 c = vec2(0.5);\n"
"  vec2 pos = fract(uv * u_fontwh);\n"
"  vec2 duv = vec2(1.0) / u_fontwh;\n"
"  float s  = texture2D(u_diffuse, uv).r;\n"
"  float l  = texture2D(u_diffuse, uv+vec2(-duv.x,0.0)).r;\n"
"  float r  = texture2D(u_diffuse, uv+vec2(+duv.x,0.0)).r;\n"
"  float t  = texture2D(u_diffuse, uv+vec2(0.0, duv.y)).r;\n"
"  float b  = texture2D(u_diffuse, uv+vec2(0.0,-duv.y)).r;\n"
"  float tl = texture2D(u_diffuse, uv+vec2(-duv.x, duv.y)).r;\n"
"  float tr = texture2D(u_diffuse, uv+vec2( duv.x, duv.y)).r;\n"
"  float bl = texture2D(u_diffuse, uv+vec2(-duv.x,-duv.y)).r;\n"
"  float br = texture2D(u_diffuse, uv+vec2( duv.x,-duv.y)).r;\n"
"  float dist = 1.0;\n"
"  if (s != 0.0) {\n"
"    dist = distance(pos,c);\n"
"    if (l!=0.0) distseg(dist, c, vec2(-0.5,0.5), vec2(0.0,1.0), pos);\n"
"    if (r!=0.0) distseg(dist, c, vec2(+1.5,0.5), vec2(0.0,1.0), pos);\n"
"    if (t!=0.0) distseg(dist, c, vec2(0.5,+1.5), vec2(1.0,0.0), pos);\n"
"    if (b!=0.0) distseg(dist, c, vec2(0.5,-0.5), vec2(1.0,0.0), pos);\n"
"    if (l==0.0&&t==0.0&&tl!=0.0) distseg(dist, c, vec2(-0.5,+1.5), vec2(+RSQ2,RSQ2), pos);\n"
"    if (r==0.0&&t==0.0&&tr!=0.0) distseg(dist, c, vec2(+1.5,+1.5), vec2(-RSQ2,RSQ2), pos);\n"
"    if (l==0.0&&b==0.0&&bl!=0.0) distseg(dist, c, vec2(-0.5,-0.5), vec2(-RSQ2,RSQ2), pos);\n"
"    if (r==0.0&&b==0.0&&br!=0.0) distseg(dist, c, vec2(+1.5,-0.5), vec2(+RSQ2,RSQ2), pos);\n"
"  } else {\n"
"    if (t!=0.0&&r!=0.0&&tr==0.0) distseg(dist, vec2(0.5,+1.5), vec2(+1.5,0.5), vec2(+RSQ2,+RSQ2), pos);\n"
"    if (t!=0.0&&l!=0.0&&tl==0.0) distseg(dist, vec2(0.5,+1.5), vec2(-0.5,0.5), vec2(-RSQ2,+RSQ2), pos);\n"
"    if (b!=0.0&&r!=0.0&&br==0.0) distseg(dist, vec2(0.5,-0.5), vec2(+1.5,0.5), vec2(-RSQ2,+RSQ2), pos);\n"
"    if (b!=0.0&&l!=0.0&&bl==0.0) distseg(dist, vec2(0.5,-0.5), vec2(-0.5,0.5), vec2(+RSQ2,+RSQ2), pos);\n"
"  }\n"
"  float no = u_font_thickness-u_outline_width;\n"
"  float o = u_font_thickness;\n"
"  vec4 col = vec4(1.0-smoothstep(no-0.1, no, dist));\n"
"  col += u_outline_color * (1.0-smoothstep(o-0.1, o, dist));\n"
"  SWITCH_WEBGL(gl_FragColor, rt_col) = col;\n"
"}\n"
};

const char forward_fp[] = {
"uniform sampler2DRect u_lighttex;\n"
"uniform sampler2DRect u_nortex;\n"
"uniform vec2 u_subbufferdim;\n"
"uniform vec2 u_rcpsubbufferdim;\n"
"PS_IN vec3 fs_nor;\n"
"IF_NOT_WEBGL(out vec4 rt_col);\n"

"#define SAMPLE(NUM, X, Y) \\n"
"  vec2 uv##NUM = uv+vec2(X,Y);\\n"
"  vec2 bufindex##NUM = mod(uv##NUM, SPLITNUM);\\n"
"  vec2 pixindex##NUM = uv##NUM / vec2(SPLITNUM);\\n"
"  vec2 unsplituv##NUM = pixindex##NUM + bufindex##NUM * u_subbufferdim;\\n"
"  vec3 col##NUM = texture2DRect(u_lighttex, unsplituv##NUM).xyz;\\n"
"  vec2 nor##NUM = texture2DRect(u_nortex, uv##NUM).xy;\\n"
"  bool ok##NUM = dot(nor##NUM, nor) > 0.8;\\n"
"  col += ok##NUM ? col##NUM : vec3(0.0);\\n"
"  num += ok##NUM ? 1.0 : 0.0;\n"

"void main() {\n"
"  vec2 uv = floor(gl_FragCoord.xy);\n"
"  vec3 col = vec3(0.0);\n"
"  vec2 nor = fs_nor.xy;\n"
"  float num = 0.0;\n"
"  SAMPLE(0,  -1.0, +1.0);\n"
"  SAMPLE(1,  -1.0,  0.0);\n"
"  SAMPLE(2,  -1.0, -1.0);\n"
"  SAMPLE(3,  -1.0, -2.0);\n"
"  SAMPLE(4,   0.0, +1.0);\n"
"  SAMPLE(5,   0.0,  0.0);\n"
"  SAMPLE(6,   0.0, -1.0);\n"
"  SAMPLE(7,   0.0, -2.0);\n"
"  SAMPLE(8,  +1.0, +1.0);\n"
"  SAMPLE(9,  +1.0,  0.0);\n"
"  SAMPLE(10, +1.0, -1.0);\n"
"  SAMPLE(11, +1.0, -2.0);\n"
"  SAMPLE(12, +2.0, +1.0);\n"
"  SAMPLE(13, +2.0,  0.0);\n"
"  SAMPLE(14, +2.0, -1.0);\n"
"  SAMPLE(15, +2.0, -2.0);\n"
"  SWITCH_WEBGL(gl_FragColor, rt_col) = vec4(col, 1.0);\n"
"}\n"
};

const char forward_vp[] = {
"uniform mat4 u_mvp;\n"
"VS_IN vec3 vs_pos;\n"
"VS_IN vec3 vs_nor;\n"
"VS_OUT vec3 fs_nor;\n"

"void main() {\n"
"  fs_nor = vs_nor;\n"
"  gl_Position = u_mvp*vec4(vs_pos,1.0);\n"
"}\n"
};

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//
const char noise2D[] = {
"vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }\n"

"float snoise(vec2 v) {\n"
"  const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0\n"
"                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)\n"
"                      -0.577350269189626, // -1.0 + 2.0 * C.x\n"
"                      0.024390243902439); // 1.0 / 41.0\n"
"  // First corner\n"
"  vec2 i  = floor(v + dot(v, C.yy));\n"
"  vec2 x0 = v -   i + dot(i, C.xx);\n"

"  // Other corners\n"
"  vec2 i1;\n"
"  //i1.x = step(x0.y, x0.x); // x0.x > x0.y ? 1.0 : 0.0\n"
"  //i1.y = 1.0 - i1.x;\n"
"  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
"  // x0 = x0 - 0.0 + 0.0 * C.xx ;\n"
"  // x1 = x0 - i1 + 1.0 * C.xx ;\n"
"  // x2 = x0 - 1.0 + 2.0 * C.xx ;\n"
"  vec4 x12 = x0.xyxy + C.xxzz;\n"
"  x12.xy -= i1;\n"

"  // Permutations\n"
"  i = mod289(i); // Avoid truncation effects in permutation\n"
"  vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));\n"

"  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);\n"
"  m = m*m ;\n"
"  m = m*m ;\n"

"  // Gradients: 41 points uniformly over a line, mapped onto a diamond.\n"
"  // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)\n"
"  vec3 x = 2.0 * fract(p * C.www) - 1.0;\n"
"  vec3 h = abs(x) - 0.5;\n"
"  vec3 ox = floor(x + 0.5);\n"
"  vec3 a0 = x - ox;\n"

"  // Normalise gradients implicitly by scaling m\n"
"  // Approximation of: m *= inversesqrt(a0*a0 + h*h);\n"
"  m *= 1.79284291400159 - 0.85373472095314 * (a0*a0 + h*h);\n"

"  // Compute final noise value at P\n"
"  vec3 g;\n"
"  g.x  = a0.x  * x0.x  + h.x  * x0.y;\n"
"  g.yz = a0.yz * x12.xz + h.yz * x12.yw;\n"
"  return 130.0 * dot(m, g);\n"
"}\n"
};

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//
const char noise3D[] = {
"vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }\n"
"vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }\n"

"float snoise(vec3 v) {\n"
"  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;\n"
"  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);\n"

"  // First corner\n"
"  vec3 i  = floor(v + dot(v, C.yyy));\n"
"  vec3 x0 =   v - i + dot(i, C.xxx) ;\n"

"  // Other corners\n"
"  vec3 g = step(x0.yzx, x0.xyz);\n"
"  vec3 l = 1.0 - g;\n"
"  vec3 i1 = min(g.xyz, l.zxy);\n"
"  vec3 i2 = max(g.xyz, l.zxy);\n"

"  //   x0 = x0 - 0.0 + 0.0 * C.xxx;\n"
"  //   x1 = x0 - i1  + 1.0 * C.xxx;\n"
"  //   x2 = x0 - i2  + 2.0 * C.xxx;\n"
"  //   x3 = x0 - 1.0 + 3.0 * C.xxx;\n"
"  vec3 x1 = x0 - i1 + C.xxx;\n"
"  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y\n"
"  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y\n"

"  // Permutations\n"
"  i = mod289(i);\n"
"  vec4 p = permute(permute(permute(\n"
"           i.z + vec4(0.0, i1.z, i2.z, 1.0))\n"
"         + i.y + vec4(0.0, i1.y, i2.y, 1.0))\n"
"         + i.x + vec4(0.0, i1.x, i2.x, 1.0));\n"

"  // Gradients: 7x7 points over a square, mapped onto an octahedron.\n"
"  // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)\n"
"  float n_ = 0.142857142857; // 1.0/7.0\n"
"  vec3  ns = n_ * D.wyz - D.xzx;\n"

"  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)\n"

"  vec4 x_ = floor(j * ns.z);\n"
"  vec4 y_ = floor(j - 7.0 * x_);    // mod(j,N)\n"

"  vec4 x = x_ *ns.x + ns.yyyy;\n"
"  vec4 y = y_ *ns.x + ns.yyyy;\n"
"  vec4 h = 1.0 - abs(x) - abs(y);\n"

"  vec4 b0 = vec4(x.xy, y.xy);\n"
"  vec4 b1 = vec4(x.zw, y.zw);\n"

"  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;\n"
"  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;\n"
"  vec4 s0 = floor(b0)*2.0 + 1.0;\n"
"  vec4 s1 = floor(b1)*2.0 + 1.0;\n"
"  vec4 sh = -step(h, vec4(0.0));\n"

"  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;\n"
"  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;\n"

"  vec3 p0 = vec3(a0.xy,h.x);\n"
"  vec3 p1 = vec3(a0.zw,h.y);\n"
"  vec3 p2 = vec3(a1.xy,h.z);\n"
"  vec3 p3 = vec3(a1.zw,h.w);\n"

"  //Normalise gradients\n"
"  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));\n"
"  p0 *= norm.x;\n"
"  p1 *= norm.y;\n"
"  p2 *= norm.z;\n"
"  p3 *= norm.w;\n"

"  // Mix final noise value\n"
"  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);\n"
"  m = m * m;\n"
"  return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));\n"
"}\n"
};

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//
const char noise4D[] = {
"vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"float mod289(float x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }\n"
"vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }\n"
"float permute(float x) { return mod289(((x*34.0)+1.0)*x); }\n"
"vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }\n"
"float taylorInvSqrt(float r) { return 1.79284291400159 - 0.85373472095314 * r; }\n"

"vec4 grad4(float j, vec4 ip) {\n"
"  const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);\n"
"  vec4 p,s;\n"
"  p.xyz = floor(fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;\n"
"  p.w = 1.5 - dot(abs(p.xyz), ones.xyz);\n"
"  s = vec4(lessThan(p, vec4(0.0)));\n"
"  p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www;\n"
"  return p;\n"
"}\n"

"// (sqrt(5) - 1)/4 = F4, used once below\n"
"#define F4 0.309016994374947451\n"

"float snoise(vec4 v) {\n"
"  const vec4  C = vec4(0.138196601125011,   // (5 - sqrt(5))/20  G4\n"
"                       0.276393202250021,   // 2 * G4\n"
"                       0.414589803375032,   // 3 * G4\n"
"                       -0.447213595499958); // -1 + 4 * G4\n"

"  // First corner\n"
"  vec4 i  = floor(v + dot(v, vec4(F4)));\n"
"  vec4 x0 = v -   i + dot(i, C.xxxx);\n"

"  // Other corners\n"

"  // Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)\n"
"  vec4 i0;\n"
"  vec3 isX = step(x0.yzw, x0.xxx);\n"
"  vec3 isYZ = step(x0.zww, x0.yyz);\n"
"  //  i0.x = dot(isX, vec3(1.0));\n"
"  i0.x = isX.x + isX.y + isX.z;\n"
"  i0.yzw = 1.0 - isX;\n"
"  //  i0.y += dot(isYZ.xy, vec2(1.0));\n"
"  i0.y += isYZ.x + isYZ.y;\n"
"  i0.zw += 1.0 - isYZ.xy;\n"
"  i0.z += isYZ.z;\n"
"  i0.w += 1.0 - isYZ.z;\n"

"  // i0 now contains the unique values 0,1,2,3 in each channel\n"
"  vec4 i3 = clamp(i0, 0.0, 1.0);\n"
"  vec4 i2 = clamp(i0-1.0, 0.0, 1.0);\n"
"  vec4 i1 = clamp(i0-2.0, 0.0, 1.0);\n"

"  //  x0 = x0 - 0.0 + 0.0 * C.xxxx\n"
"  //  x1 = x0 - i1  + 1.0 * C.xxxx\n"
"  //  x2 = x0 - i2  + 2.0 * C.xxxx\n"
"  //  x3 = x0 - i3  + 3.0 * C.xxxx\n"
"  //  x4 = x0 - 1.0 + 4.0 * C.xxxx\n"
"  vec4 x1 = x0 - i1 + C.xxxx;\n"
"  vec4 x2 = x0 - i2 + C.yyyy;\n"
"  vec4 x3 = x0 - i3 + C.zzzz;\n"
"  vec4 x4 = x0 + C.wwww;\n"

"  // Permutations\n"
"  i = mod289(i);\n"
"  float j0 = permute(permute(permute(permute(i.w) + i.z) + i.y) + i.x);\n"
"  vec4 j1 = permute(permute(permute(permute(\n"
"             i.w + vec4(i1.w, i2.w, i3.w, 1.0))\n"
"           + i.z + vec4(i1.z, i2.z, i3.z, 1.0))\n"
"           + i.y + vec4(i1.y, i2.y, i3.y, 1.0))\n"
"           + i.x + vec4(i1.x, i2.x, i3.x, 1.0));\n"

"  // Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytope\n"
"  // 7*7*6 = 294, which is close to the ring size 17*17 = 289.\n"
"  vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;\n"

"  vec4 p0 = grad4(j0,   ip);\n"
"  vec4 p1 = grad4(j1.x, ip);\n"
"  vec4 p2 = grad4(j1.y, ip);\n"
"  vec4 p3 = grad4(j1.z, ip);\n"
"  vec4 p4 = grad4(j1.w, ip);\n"

"  // Normalise gradients\n"
"  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));\n"
"  p0 *= norm.x;\n"
"  p1 *= norm.y;\n"
"  p2 *= norm.z;\n"
"  p3 *= norm.w;\n"
"  p4 *= taylorInvSqrt(dot(p4,p4));\n"

"  // Mix contributions from the five corners\n"
"  vec3 m0 = max(0.6 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);\n"
"  vec2 m1 = max(0.6 - vec2(dot(x3,x3), dot(x4,x4)           ), 0.0);\n"
"  m0 = m0 * m0;\n"
"  m1 = m1 * m1;\n"
"  return 49.0 * (dot(m0*m0, vec3(dot(p0, x0), dot(p1, x1), dot(p2, x2)))\n"
"      + dot(m1*m1, vec2(dot(p3, x3), dot(p4, x4)))) ;\n"
"}\n"
};
} /* namespace q */
} /* namespace shaders */

