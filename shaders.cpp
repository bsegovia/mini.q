/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> stores shaders (do not modify)
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {
 const char fixed_fp[] = {
"#if USE_DIFFUSETEX\n"
"uniform sampler2D u_diffuse;\n"
"PS_IN vec2 fs_tex;\n"
"#endif\n"
"#if USE_FOG\n"
"uniform vec4 u_fogcolor;\n"
"uniform vec2 u_fogstartend;\n"
"PS_IN float fs_fogz;\n"
"#endif\n"
"PS_IN vec4 fs_col;\n"
"IF_NOT_WEBGL(out vec4 rt_c);\n"

"void main() {\n"
"  vec4 col;\n"
"#if USE_DIFFUSETEX\n"
"  col = texture2D(u_diffuse, fs_tex);\n"
"  col *= fs_col;\n"
"#else\n"
"  col = fs_col;\n"
"#endif\n"
"#if USE_FOG\n"
"  float factor = clamp((-fs_fogz-u_fogstartend.x)*u_fogstartend.y,0.0,1.0);\n"
"  col.xyz = mix(col.xyz,u_fogcolor.xyz,factor);\n"
"#endif\n"
"  SWITCH_WEBGL(gl_FragColor = col;, rt_c = col;)\n"
"}\n"
 };

 const char fixed_vp[] = {
"uniform mat4 u_mvp;\n"
"#if USE_FOG\n"
"uniform vec4 u_zaxis;\n"
"VS_OUT float fs_fogz;\n"
"#endif\n"
"#if USE_KEYFRAME\n"
"uniform float u_delta;\n"
"VS_IN vec3 vs_pos0, vs_pos1;\n"
"#else\n"
"VS_IN vec3 vs_pos;\n"
"#endif\n"
"VS_IN vec4 vs_col;\n"
"#if USE_DIFFUSETEX\n"
"VS_IN vec2 vs_tex;\n"
"VS_OUT vec2 fs_tex;\n"
"#endif\n"
"VS_OUT vec4 fs_col;\n"
"void main() {\n"
"#if USE_DIFFUSETEX\n"
"  fs_tex = vs_tex;\n"
"#endif\n"
"  fs_col = vs_col;\n"
"#if USE_KEYFRAME\n"
"  vec3 vs_pos = mix(vs_pos0,vs_pos1,u_delta);\n"
"#endif\n"
"#if USE_FOG\n"
"  fs_fogz = dot(u_zaxis.xyz,vs_pos)+u_zaxis.w;\n"
"#endif\n"
"  gl_Position = u_mvp*vec4(vs_pos,1.0);\n"
"}\n"
 };

 const char font_fp[] = {
"uniform sampler2D u_diffuse;\n"
"PS_IN vec2 fs_tex;\n"
"IF_NOT_WEBGL(out vec4 rt_c);\n"

"void main() {\n"
"  vec4 col = smoothstep(0.35,0.45,texture2D(u_diffuse, fs_tex));\n"
"  SWITCH_WEBGL(gl_FragColor = col;, rt_c = col;)\n"
"}\n"
 };

} /* namespace q */
} /* namespace shaders */

