/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> stores shaders (do not modify)
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {
const char debugunsplit_fp[] = {
"uniform sampler2DRect u_nortex;\n"
"IF_NOT_WEBGL(out vec4 rt_c);\n"

"void main() {\n"
"  vec2 uv = gl_FragCoord.xy;\n"
"  vec2 bufindex = uv / vec2(320.0,256.0);\n"
"  vec2 pixindex = mod(uv, vec2(320.0,256.0));\n"
"  vec2 interleaveduv = 4.0 * pixindex + bufindex;\n"
"  vec4 nor = texture2DRect(u_nortex, uv);\n"
"  //vec4 nor = texture2DRect(u_nortex, interleaveduv);\n"
"  SWITCH_WEBGL(gl_FragColor = abs(nor), rt_c = abs(nor));\n"
"}\n"
};

const char debugunsplit_vp[] = {
"uniform mat4 u_mvp;\n"
"VS_IN vec2 vs_pos;\n"
"void main() {gl_Position = u_mvp*vec4(vs_pos,1.0,1.0);}\n"
};

const char deferred_fp[] = {
"uniform sampler2DRect u_nortex;\n"
"IF_NOT_WEBGL(out vec4 rt_c);\n"

"void main() {\n"
"  vec2 uv = gl_FragCoord.xy;\n"
"  vec2 bufindex = uv / vec2(320.0,256.0);\n"
"  vec2 pixindex = mod(uv, vec2(320.0,256.0));\n"
"  vec2 interleaveduv = 4.0 * pixindex + bufindex;\n"
"  vec4 nor = texture2DRect(u_nortex, uv);\n"
"  //vec4 nor = texture2DRect(u_nortex, interleaveduv);\n"
"  SWITCH_WEBGL(gl_FragColor = abs(nor), rt_c = abs(nor));\n"
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
"IF_NOT_WEBGL(out vec4 rt_c);\n"

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
"  SWITCH_WEBGL(gl_FragColor = col, rt_c = col);\n"
"  // SWITCH_WEBGL(gl_FragColor = col, rt_c = vec4(1.0,0.0,0.0,0.0));\n"
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
"IF_NOT_WEBGL(out vec4 rt_c);\n"

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
"  SWITCH_WEBGL(gl_FragColor = col, rt_c = col);\n"
"}\n"
};

} /* namespace q */
} /* namespace shaders */

