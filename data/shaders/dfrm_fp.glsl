//## const char dfrm_fp[] = {

/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - shader 'Hard Edge Shadow' by gltracy from www.shadertoy.com
 -------------------------------------------------------------------------*/
IF_NOT_WEBGL(out vec4 rt_c);
uniform vec3 iResolution;
uniform float iGlobalTime;
const int max_iterations = 255;
const float stop_threshold = 0.001;
const float step_scale = 0.5;
const float grad_step = 0.1;
const float clip_far = 1000.0;

// math const
const float PI = 3.14159265359;
const float DEG_TO_RAD = PI / 180.0;

// math
mat2 rot2( float angle ) {
  float c = cos( angle );
  float s = sin( angle );
  return mat2(c, s, -s, c);
}

// pitch, yaw
mat3 rot3xy( vec2 angle ) {
  vec2 c = cos( angle );
  vec2 s = sin( angle );

  return mat3(
      c.y      ,  0.0, -s.y,
      s.y * s.x,  c.x,  c.y * s.x,
      s.y * c.x, -s.x,  c.y * c.x);
}

// distance function
float dist_sphere( vec3 v, float r ) {
  return length( v ) - r;
}

float dist_box( vec3 v, vec3 size, float r ) {
  return length( max( abs( v ) - size, 0.0 ) ) - r;
}

// get distance in the world
float dist_field( vec3 v ) {
  // ...add objects here...

  // box
  float d2 = dist_box( v + vec3( 0.0, 2.2, 0.0 ), vec3( 8.0, 0.05, 8.0 ), 0.05);

  // twist
  v.xz = rot2( v.y ) * v.xz;

  // sphere
  float d0 = dist_sphere( v, 2.7 );

  // box
  float d1 = dist_box( v, vec3( 2.0, 2.0, 2.0 ), 0.05 );


  // union     : min( d0,  d1 )
  // intersect : max( d0,  d1 )
  // subtract  : max( d1, -d0 )
  return min( d2, max( d1, -d0 ) );
}

// get gradient in the world
vec3 gradient( vec3 v ) {
  const vec3 dx = vec3( grad_step, 0.0, 0.0 );
  const vec3 dy = vec3( 0.0, grad_step, 0.0 );
  const vec3 dz = vec3( 0.0, 0.0, grad_step );
  return normalize (
      vec3(
        dist_field( v + dx ) - dist_field( v - dx ),
        dist_field( v + dy ) - dist_field( v - dy ),
        dist_field( v + dz ) - dist_field( v - dz )));
}

// ray marching
float ray_marching( vec3 origin, vec3 dir, float start, float end ) {
  float depth = start;
  for ( int i = 0; i < max_iterations; i++ ) {
    float dist = dist_field( origin + dir * depth );
    if ( dist < stop_threshold ) {
      return depth;
    }
    depth += dist * step_scale;
    if ( depth >= end) {
      return end;
    }
  }
  return end;
}

// get ray direction
vec3 ray_dir( float fov, vec2 size, vec2 pos ) {
  vec2 xy = pos - size * 0.5;
  float cot_half_fov = tan( ( 90.0 - fov * 0.5 ) * DEG_TO_RAD );  
  float z = size.y * 0.5 * cot_half_fov;
  return normalize( vec3( xy, -z ) );
}

// shadow
float shadow( vec3 light, vec3 lv, float len ) {
  float depth = ray_marching( light, lv, 0.0, len );
  return step( len - depth, 0.01 );
}

// phong shading
vec3 shading( vec3 v, vec3 n, vec3 eye ) {
  // ...add lights here...
  const float shininess = 64.0;

  vec3 final = vec3( 0.0 );

  vec3 ev = normalize( v - eye );
  vec3 ref_ev = reflect( ev, n );

  // light 0
  {
    vec3 light_pos = vec3( 7.0, 7.0, 7.0 );
    vec3 light_color = vec3( 0.9, 0.7, 0.4 );

    vec3 vl = light_pos - v;
    float len = length( vl );
    vl /= len;

    float att = min( 100.0 / ( len * len ), 1.0 );
    float diffuse  = max( 0.0, dot( vl, n) );
    float specular = max( 0.0, dot( vl, ref_ev ) );
    specular = pow( specular, shininess );
    final += light_color * (att * shadow(light_pos, -vl, len ) * ( diffuse * 0.6 + specular * 0.5 ));
  }

  // light 1
  {
    vec3 light_pos   = vec3( 0.0, 1.0, 0.0 );
    vec3 light_color = vec3( 0.3, 0.7, 1.0 );

    vec3 vl = light_pos - v;
    float len = length( vl );
    vl /= len;

    float att = min( 7.0 / ( len * len ), 1.0 );
    float diffuse  = max( 0.0, dot( vl, n ) );
    float specular = max( 0.0, dot( vl, ref_ev ) ); specular = pow( specular, shininess );
    final += light_color * ( att * shadow( light_pos, -vl, len ) * ( diffuse * 0.9 + specular * 0.9 ));
  }

  // light 2
  {
    vec3 light_pos   = vec3( -7.0, 7.0, -7.0 );
    vec3 light_color = vec3( 0.6, 0.3, 0.6 );

    vec3 vl = light_pos - v;
    float len = length( vl );
    vl /= len;

    float att = min( 122.0 / ( len * len), 1.0 );
    float diffuse  = max( 0.0, dot( vl, n ) );
    float specular = max( 0.0, dot( vl, ref_ev ) );
    specular = pow( specular, shininess );
    final += light_color * (att * shadow( light_pos, -vl, len ) * ( diffuse * 0.6 + specular * 0.5 ));
  }

  return final;
}

void main(void)
{
  // default ray dir
  vec3 dir = ray_dir( 45.0, iResolution.xy, gl_FragCoord.xy );

  // default ray origin
  vec3 eye = vec3( 0.0, 0.0, 12.0 );

  // rotate camera
  mat3 rot = rot3xy( vec2( -DEG_TO_RAD*30.0, iGlobalTime * 0.5 ) );
  dir = rot * dir;
  eye = rot * eye;

  // ray marching
  float depth = ray_marching( eye, dir, 0.0, clip_far );
  if ( depth >= clip_far ) {
    discard;
  }

  // shading
  vec3 pos = eye + dir * depth;
  vec3 n = gradient( pos );
  rt_c = vec4( shading( pos, n, eye ), 1.0 );
}
//## };

