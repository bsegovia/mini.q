/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shaders.cpp -> stores shaders (do not modify)
 -------------------------------------------------------------------------*/
#include "shaders.hpp"
namespace q {
namespace shaders {
 const char dfrm_fp[] = {
"/*-------------------------------------------------------------------------\n"
" - mini.q - a minimalistic multiplayer FPS\n"
" - shader 'Hard Edge Shadow' by gltracy from www.shadertoy.com\n"
" -------------------------------------------------------------------------*/\n"
"IF_NOT_WEBGL(out vec4 rt_c);\n"
"uniform vec3 iResolution;\n"
"uniform float iGlobalTime;\n"
"const int max_iterations = 255;\n"
"const float stop_threshold = 0.001;\n"
"const float step_scale = 0.5;\n"
"const float grad_step = 0.1;\n"
"const float clip_far = 1000.0;\n"

"// math const\n"
"const float PI = 3.14159265359;\n"
"const float DEG_TO_RAD = PI / 180.0;\n"

"// math\n"
"mat2 rot2( float angle ) {\n"
"  float c = cos( angle );\n"
"  float s = sin( angle );\n"
"  return mat2(c, s, -s, c);\n"
"}\n"

"// pitch, yaw\n"
"mat3 rot3xy( vec2 angle ) {\n"
"  vec2 c = cos( angle );\n"
"  vec2 s = sin( angle );\n"

"  return mat3(\n"
"      c.y      ,  0.0, -s.y,\n"
"      s.y * s.x,  c.x,  c.y * s.x,\n"
"      s.y * c.x, -s.x,  c.y * c.x);\n"
"}\n"

"// distance function\n"
"float dist_sphere( vec3 v, float r ) {\n"
"  return length( v ) - r;\n"
"}\n"

"float dist_box( vec3 v, vec3 size, float r ) {\n"
"  return length( max( abs( v ) - size, 0.0 ) ) - r;\n"
"}\n"

"// get distance in the world\n"
"float dist_field( vec3 v ) {\n"
"  // ...add objects here...\n"

"  // box\n"
"  float d2 = dist_box( v + vec3( 0.0, 2.2, 0.0 ), vec3( 8.0, 0.05, 8.0 ), 0.05);\n"

"  // twist\n"
"  v.xz = rot2( v.y ) * v.xz;\n"

"  // sphere\n"
"  float d0 = dist_sphere( v, 2.7 );\n"

"  // box\n"
"  float d1 = dist_box( v, vec3( 2.0, 2.0, 2.0 ), 0.05 );\n"


"  // union     : min( d0,  d1 )\n"
"  // intersect : max( d0,  d1 )\n"
"  // subtract  : max( d1, -d0 )\n"
"  return min( d2, max( d1, -d0 ) );\n"
"}\n"

"// get gradient in the world\n"
"vec3 gradient( vec3 v ) {\n"
"  const vec3 dx = vec3( grad_step, 0.0, 0.0 );\n"
"  const vec3 dy = vec3( 0.0, grad_step, 0.0 );\n"
"  const vec3 dz = vec3( 0.0, 0.0, grad_step );\n"
"  return normalize (\n"
"      vec3(\n"
"        dist_field( v + dx ) - dist_field( v - dx ),\n"
"        dist_field( v + dy ) - dist_field( v - dy ),\n"
"        dist_field( v + dz ) - dist_field( v - dz )));\n"
"}\n"

"// ray marching\n"
"float ray_marching( vec3 origin, vec3 dir, float start, float end ) {\n"
"  float depth = start;\n"
"  for ( int i = 0; i < max_iterations; i++ ) {\n"
"    float dist = dist_field( origin + dir * depth );\n"
"    if ( dist < stop_threshold ) {\n"
"      return depth;\n"
"    }\n"
"    depth += dist * step_scale;\n"
"    if ( depth >= end) {\n"
"      return end;\n"
"    }\n"
"  }\n"
"  return end;\n"
"}\n"

"// get ray direction\n"
"vec3 ray_dir( float fov, vec2 size, vec2 pos ) {\n"
"  vec2 xy = pos - size * 0.5;\n"
"  float cot_half_fov = tan( ( 90.0 - fov * 0.5 ) * DEG_TO_RAD );  \n"
"  float z = size.y * 0.5 * cot_half_fov;\n"
"  return normalize( vec3( xy, -z ) );\n"
"}\n"

"// shadow\n"
"float shadow( vec3 light, vec3 lv, float len ) {\n"
"  float depth = ray_marching( light, lv, 0.0, len );\n"
"  return step( len - depth, 0.01 );\n"
"}\n"

"// phong shading\n"
"vec3 shading( vec3 v, vec3 n, vec3 eye ) {\n"
"  // ...add lights here...\n"
"  const float shininess = 64.0;\n"

"  vec3 final = vec3( 0.0 );\n"

"  vec3 ev = normalize( v - eye );\n"
"  vec3 ref_ev = reflect( ev, n );\n"

"  // light 0\n"
"  {\n"
"    vec3 light_pos = vec3( 7.0, 7.0, 7.0 );\n"
"    vec3 light_color = vec3( 0.9, 0.7, 0.4 );\n"

"    vec3 vl = light_pos - v;\n"
"    float len = length( vl );\n"
"    vl /= len;\n"

"    float att = min( 100.0 / ( len * len ), 1.0 );\n"
"    float diffuse  = max( 0.0, dot( vl, n) );\n"
"    float specular = max( 0.0, dot( vl, ref_ev ) );\n"
"    specular = pow( specular, shininess );\n"
"    final += light_color * (att * shadow(light_pos, -vl, len ) * ( diffuse * 0.6 + specular * 0.5 ));\n"
"  }\n"

"  // light 1\n"
"  {\n"
"    vec3 light_pos   = vec3( 0.0, 1.0, 0.0 );\n"
"    vec3 light_color = vec3( 0.3, 0.7, 1.0 );\n"

"    vec3 vl = light_pos - v;\n"
"    float len = length( vl );\n"
"    vl /= len;\n"

"    float att = min( 7.0 / ( len * len ), 1.0 );\n"
"    float diffuse  = max( 0.0, dot( vl, n ) );\n"
"    float specular = max( 0.0, dot( vl, ref_ev ) ); specular = pow( specular, shininess );\n"
"    final += light_color * ( att * shadow( light_pos, -vl, len ) * ( diffuse * 0.9 + specular * 0.9 ));\n"
"  }\n"

"  // light 2\n"
"  {\n"
"    vec3 light_pos   = vec3( -7.0, 7.0, -7.0 );\n"
"    vec3 light_color = vec3( 0.6, 0.3, 0.6 );\n"

"    vec3 vl = light_pos - v;\n"
"    float len = length( vl );\n"
"    vl /= len;\n"

"    float att = min( 122.0 / ( len * len), 1.0 );\n"
"    float diffuse  = max( 0.0, dot( vl, n ) );\n"
"    float specular = max( 0.0, dot( vl, ref_ev ) );\n"
"    specular = pow( specular, shininess );\n"
"    final += light_color * (att * shadow( light_pos, -vl, len ) * ( diffuse * 0.6 + specular * 0.5 ));\n"
"  }\n"

"  return final;\n"
"}\n"

"void main(void)\n"
"{\n"
"  // default ray dir\n"
"  vec3 dir = ray_dir( 45.0, iResolution.xy, gl_FragCoord.xy );\n"

"  // default ray origin\n"
"  vec3 eye = vec3( 0.0, 0.0, 12.0 );\n"

"  // rotate camera\n"
"  mat3 rot = rot3xy( vec2( -DEG_TO_RAD*30.0, iGlobalTime * 0.5 ) );\n"
"  dir = rot * dir;\n"
"  eye = rot * eye;\n"

"  // ray marching\n"
"  float depth = ray_marching( eye, dir, 0.0, clip_far );\n"
"  if ( depth >= clip_far ) {\n"
"    discard;\n"
"  }\n"

"  // shading\n"
"  vec3 pos = eye + dir * depth;\n"
"  vec3 n = gradient( pos );\n"
"  rt_c = vec4( shading( pos, n, eye ), 1.0 );\n"
"}\n"
 };

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
"#if USE_FOG\n"
"  float factor = clamp((-fs_fogz-u_fogstartend.x)*u_fogstartend.y,0.0,1.0);\n"
"  col.xyz = mix(col.xyz,u_fogcolor.xyz,factor);\n"
"#endif\n"
"  SWITCH_WEBGL(gl_FragColor = col, rt_c = col);\n"
"  // SWITCH_WEBGL(gl_FragColor = col, rt_c = vec4(1.0,0.0,0.0,0.0));\n"
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

"#define FONTW 128\n"
"#define FONTH 96\n"
"#define RSQ2 0.7071078\n"

"void distseg(inout float dist, vec2 start, vec2 end, vec2 nor, vec2 pos) {\n"
"  if (dot(pos-start,end-start)>=0.0 && dot(end-pos,end-start)>=0.0)\n"
"    dist = min(dist, abs(dot(pos-start, nor)));\n"
"}\n"

"void main() {\n"
"  vec2 uv = fs_tex;\n"
"  vec2 c = vec2(0.5);\n"
"  vec2 pos = fract(uv * vec2(FONTW, FONTH));\n"
"  float du = 1.0 / float(FONTW);\n"
"  float dv = 1.0 / float(FONTH);\n"
"  float s  = texture2D(u_diffuse, uv).r;\n"
"  float l  = texture2D(u_diffuse, uv+vec2(-du, 0.0)).r;\n"
"  float r  = texture2D(u_diffuse, uv+vec2(+du, 0.0)).r;\n"
"  float t  = texture2D(u_diffuse, uv+vec2(0.0, dv)).r;\n"
"  float b  = texture2D(u_diffuse, uv+vec2(0.0,-dv)).r;\n"
"  float tl = texture2D(u_diffuse, uv+vec2(-du, dv)).r;\n"
"  float tr = texture2D(u_diffuse, uv+vec2( du, dv)).r;\n"
"  float bl = texture2D(u_diffuse, uv+vec2(-du,-dv)).r;\n"
"  float br = texture2D(u_diffuse, uv+vec2( du,-dv)).r;\n"
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
"  vec4 col = dist < 0.3 ? vec4(1.0) : vec4(0.0);\n"
"  SWITCH_WEBGL(gl_FragColor = col, rt_c = col);\n"
"}\n"
 };

} /* namespace q */
} /* namespace shaders */

