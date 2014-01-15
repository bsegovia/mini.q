/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#include "ogl.hpp"
#include "text.hpp"
#include "renderer.hpp"
#include "math.hpp"
#include "stl.hpp"
#include "shaders.hpp"

namespace q {
namespace text {

#include "font.hxx"

// ogl is responsible for the texture destruction
static u32 textfont = 0;
u32 getoglfont() { return textfont; }
static void buildfont() {
  u8 *font = (u8*)MALLOC(fontw*fonth*sizeof(u8));
  loopi(fonth) loopj(fontw/32) loopk(32)
    font[i*fontw+j*32+k] = (fontdata[i*fontw/32+j]&(1<<k))?~0x0:0x0;
  u32 id = ogl::maketex("TB Ir Dr B2 Wsr Wtr Mn mn", font, fontw, fonth);
  FREE(font);
  textfont = id;
}

// handle font shaders
static struct fontshadertype : ogl::shadertype {
  u32 u_fontw, u_fonth, u_font_thickness;
  u32 u_outline_width, u_outline_color;
} fontshader;
static void fontuniform(ogl::shadertype &s) {
  auto &df = static_cast<fontshadertype&>(s);
  OGLR(df.u_fontw, GetUniformLocation, df.program, "u_fontw");
  OGLR(df.u_fonth, GetUniformLocation, df.program, "u_fonth");
  OGLR(df.u_font_thickness, GetUniformLocation, df.program, "u_font_thickness");
  OGLR(df.u_outline_width, GetUniformLocation, df.program, "u_outline_width");
  OGLR(df.u_outline_color, GetUniformLocation, df.program, "u_outline_color");
}
static const ogl::shaderresource font_rsc = {
  "data/shaders/fixed_vp.glsl",
  "data/shaders/font_fp.glsl",
  shaders::fixed_vp,
  shaders::font_fp,
  fontuniform
};
void loadfontshader(bool fatalerr, bool fromfile) {
  using namespace ogl;
  if (!buildshader(fontshader, font_rsc, DIFFUSETEX, fromfile))
    shadererror(fatalerr, "font shader");
}
void start() {
  loadfontshader(true, false);
  buildfont();
}
void finish() { ogl::destroyshader(fontshader); }

// font parameters
static vec4f fontoutlinecolor = zero;
static float fontthickness = 0.5f, fontoutlinewidth = 0.0f;
static float in_width = float(charw), in_height = float(charh);

vec2f fontdim() { return vec2f(in_width, in_height); }
void charwidth(float w) { in_width = w; in_height = w * charh / charw; }
void thickness(float t) { fontthickness = t; }
void outlinecolor(const vec4f &c) { fontoutlinecolor = c; }
void outlinewidth(float w) { fontoutlinewidth = w; }
static void bindfontshader() {
  ogl::bindshader(fontshader);
  OGL(Uniform1f, fontshader.u_fontw, float(fontw));
  OGL(Uniform1f, fontshader.u_fonth, float(fonth));
  OGL(Uniform1f, fontshader.u_font_thickness, fontthickness);
  OGL(Uniform4fv, fontshader.u_outline_color, 1, &fontoutlinecolor.x);
  OGL(Uniform1f, fontshader.u_outline_width, fontoutlinewidth);
}

typedef u16 indextype;
static const indextype twotriangles[] = {0,1,2,0,2,3};

float width(const char *str) {
  float x = 0.f;
  for (int i = 0; str[i] != 0; ++i) {
    int c = str[i];
    if (c=='\t') { x = (x+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB; continue; }; 
    if (c=='\f') continue;
    if (c==' ') { x += in_width; continue; };
    c -= 33;
    if (c<0 || c>=95) continue;
    x += in_width;
  }
  return x;
}

void drawf(const char *fstr, const vec2f &pos, ...) {
  sprintf_sdlv(str, pos, fstr);
  draw(str, pos);
}

void draw(const char *str, const vec2f &pos) {
  OGL(BlendFunc, GL_ONE, GL_ONE);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHARACTERS));
  OGL(VertexAttrib3f,ogl::COL,1.f,1.f,1.f);

  // use a triangle mesh to display the text
  const size_t len = strlen(str);
  indextype *indices = (indextype*) alloca(len*sizeof(int[6]));
  auto verts = (vec4f*) alloca(len*sizeof(vec4f[4]));

  // traverse the string and build the mesh
  int index = 0, vert = 0;
  float x = pos.x, y = pos.y;
  for (int i = 0; str[i] != 0; ++i) {
    int c = str[i];
    if (c=='\t') { x = (x-pos.y+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB+pos.y; continue; }
    if (c=='\f') { OGL(VertexAttrib3f,ogl::COL,0.25f,1.f,0.5f); continue; }
    if (c==' ')  { x += in_width; continue; }
    c -= 32;
    if (c<0 || c>=95) continue;
    const float in_left   = (float(c%fontcol)*float(charw)-0.05f) / float(fontw);
    const float in_top    = (float(c/fontcol)*float(charh)-0.05f) / float(fonth);
    const float in_right  = in_left + (float(charw)-0.05f)/float(fontw);
    const float in_bottom = in_top + (float(charh)-0.05f)/float(fonth);

    loopj(6) indices[index+j] = vert+twotriangles[j];
    verts[vert+0] = vec4f(in_left, in_top,   x,         y);
    verts[vert+1] = vec4f(in_right,in_top,   x+in_width,y);
    verts[vert+2] = vec4f(in_right,in_bottom,x+in_width,y+in_height);
    verts[vert+3] = vec4f(in_left, in_bottom,x,         y+in_height);

    x += in_width;
    index += 6;
    vert += 4;
  }
  bindfontshader();
  ogl::immdrawelememts("Tst2p2", index, indices, &verts[0].x);
}
} /* namespace text */
} /* namespace q */

