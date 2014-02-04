//##const char deferred_vp[] = {
uniform mat4 u_mvp;
VS_IN vec2 vs_pos;
void main() {gl_Position = u_mvp*vec4(vs_pos,1.0,1.0);}
//##};

