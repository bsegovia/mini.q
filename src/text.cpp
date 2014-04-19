/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#include "ogl.hpp"
#include "text.hpp"
#include "renderer.hpp"
#include "shaders.hpp"
#include "base/math.hpp"
#include "base/string.hpp"
#include "base/stl.hpp"

namespace q {
namespace text {

#include "font.hxx"

// ogl is responsible for the texture destruction
static u32 textfont = 0;
u32 oglfont() { return textfont; }
static void buildfont() {
  auto font = (u8*)MALLOC(fontw*fonth*sizeof(u8));
  loopi(fonth) loopj(fontw/32) loopk(32)
    font[i*fontw+j*32+k] = (fontdata[i*fontw/32+j]&(1<<k))?~0x0:0x0;
  u32 id = ogl::maketex("TB Ir Dr B2 Wsr Wtr Mn mn", font, fontw, fonth);
  FREE(font);
  textfont = id;
}

static void fontrules(ogl::shaderrules &vert, ogl::shaderrules &frag, u32) {
  ogl::fixedrules(vert,frag,ogl::FIXED_DIFFUSETEX|ogl::FIXED_COLOR);
}
#define RULES fontrules
#define SHADERNAME font
#define VERTEX_PROGRAM "data/shaders/fixed_vp.decl"
#define FRAGMENT_PROGRAM "data/shaders/font_fp.decl"
#include "shaderdecl.hxx"
#undef RULES

void start() {
  buildfont();
  font::s.fixedfunction = true;
}
#if !defined(RELEASE)
void finish() {}
#endif

// font parameters
static vec4f fontoutlinecolor = zero;
static float fontthickness = 0.5f, fontoutlinewidth = 0.0f;
static float displayw = float(charw), displayh = float(charh);

vec2f fontdim() { return charwh; }
vec2f displaydim() { return vec2f(displayw, displayh); }
float fontratio() { return float(charh) / float(charw); }
void displaywidth(float w) { displayw = w; displayh = w * fontratio(); }
void thickness(float t) { fontthickness = t; }
void outlinecolor(const vec4f &c) { fontoutlinecolor = c; }
void outlinewidth(float w) { fontoutlinewidth = w; }
void resetdefaultwidth() {displaywidth(float(charw));}
static void bindfontshader() {
  ogl::bindshader(font::s);
  OGL(Uniform2f, font::s.u_fontwh, float(fontw), float(fonth));
  OGL(Uniform1f, font::s.u_font_thickness, fontthickness);
  OGL(Uniform4fv, font::s.u_outline_color, 1, &fontoutlinecolor.x);
  OGL(Uniform1f, font::s.u_outline_width, fontoutlinewidth);
}

typedef u16 indextype;
static const indextype twotriangles[] = {0,1,2,0,2,3};

float width(const char *str) {
  float x = 0.f;
  for (int i = 0; str[i] != 0; ++i) {
    int c = str[i];
    if (c=='\t') { x = (x+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB; continue; }; 
    if (c=='\f') continue;
    if (c==' ') { x += displayw; continue; };
    c -= 33;
    if (c<0 || c>=95) continue;
    x += displayw;
  }
  return x;
}

void drawf(const char *fstr, vec2f pos, ...) {
  sprintf_sdlv(str, pos, fstr);
  draw(str, pos);
}

void drawf(const char *fstr, float x, float y, ...) {
  sprintf_sdlv(str, y, fstr);
  draw(str, x, y);
}

void draw(const char *str, vec2f pos) {
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHARACTERS));

  // use a triangle mesh to display the text
  const auto len = strlen(str);
  auto indices = (indextype*) alloca(len*sizeof(int[6]));
  auto verts = (vec4f*) alloca(len*sizeof(vec4f[4]));

  // traverse the string and build the mesh
  auto index = 0, vert = 0;
  auto x = pos.x, y = pos.y;
  for (int i = 0; str[i] != 0; ++i) {
    int c = str[i];
    if (c=='\t') {x = (x-pos.y+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB+pos.y; continue;}
    if (c=='\f') {OGL(VertexAttrib3f,ogl::ATTRIB_COL,0.25f,1.f,0.5f); continue;}
    if (c==' ')  {x += displayw; continue;}
    c -= 32;
    if (c<0 || c>=95) continue;
    const float EPS = 1e-3f;
    const auto in_left   = (float(c%fontcol)*float(charw)-EPS) / float(fontw);
    const auto in_top    = (float(c/fontcol)*float(charh)-EPS) / float(fonth);
    const auto in_right  = in_left + (float(charw)-EPS)/float(fontw);
    const auto in_bottom = in_top + (float(charh)-EPS)/float(fonth);

    loopj(6) indices[index+j] = vert+twotriangles[j];
    verts[vert+0] = vec4f(in_left, in_bottom,   x,         y);
    verts[vert+1] = vec4f(in_right,in_bottom,   x+displayw,y);
    verts[vert+2] = vec4f(in_right,in_top,x+displayw,y+displayh);
    verts[vert+3] = vec4f(in_left, in_top,x,         y+displayh);

    x += displayw;
    index += 6;
    vert += 4;
  }
  bindfontshader();
  ogl::immdrawelememts("Tst2p2", index, indices, &verts[0].x);
}
} /* namespace text */
} /* namespace q */

