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
static u32 in_width = charw * rr::FONTH / charh, in_height = rr::FONTH;

void charwidth(u32 w) { in_width = w; in_height = w * charh / charw; }
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

int width(const char *str) {
  int x = 0;
  for (int i = 0; str[i] != 0; i++) {
    int c = str[i];
    if (c=='\t') { x = (x+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB; continue; }; 
    if (c=='\f') continue;
    if (c==' ') { x += in_height; continue; };
    c -= 33;
    if (c<0 || c>=95) continue;
    x += in_width + 1;
  }
  return x;
}

void drawf(const char *fstr, int left, int top, ...) {
  sprintf_sdlv(str, top, fstr);
  draw(str, left, top);
}

void draw(const char *str, int left, int top) {
  OGL(BlendFunc, GL_ONE, GL_ONE);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHARACTERS));
  OGL(VertexAttrib3f,ogl::COL,1.f,1.f,1.f);

  // use a triangle mesh to display the text
  const size_t len = strlen(str);
  indextype *indices = (indextype*) alloca(6*len*sizeof(int));
  auto verts = (vec4f*) alloca(4*len*sizeof(vec4f));

  // traverse the string and build the mesh
  int index = 0, vert = 0, x = left, y = top;
  for (int i = 0; str[i] != 0; ++i) {
    int c = str[i];
    if (c=='\t') { x = (x-left+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB+left; continue; }
    if (c=='\f') { OGL(VertexAttrib3f,ogl::COL,0.25f,1.f,0.5f); continue; }
    if (c==' ') { x += in_height; continue; }
    c -= 32;
    if (c<0 || c>=95) continue;
    const float in_left   = (float(c%fontcol)*float(charw)) / float(fontw);
    const float in_top    = (float(c/fontcol)*float(charh)) / float(fonth);
    const float in_right  = in_left + (float(charw))/float(fontw);
    const float in_bottom = in_top + (float(charh)-0.5f)/float(fonth);

    loopj(6) indices[index+j] = vert+twotriangles[j];
    verts[vert+0] = vec4f(in_left, in_top,   float(x),         float(y));
    verts[vert+1] = vec4f(in_right,in_top,   float(x+in_width),float(y));
    verts[vert+2] = vec4f(in_right,in_bottom,float(x+in_width),float(y+in_height));
    verts[vert+3] = vec4f(in_left, in_bottom,float(x),         float(y+in_height));

    x += in_width + 1;
    index += 6;
    vert += 4;
  }
  bindfontshader();
  ogl::immdrawelememts("Tst2p2", index, indices, &verts[0].x);
}

static void drawenvboxface(float s0, float t0, int x0, int y0, int z0,
                           float s1, float t1, int x1, int y1, int z1,
                           float s2, float t2, int x2, int y2, int z2,
                           float s3, float t3, int x3, int y3, int z3,
                           int texture) {
  const array<float,5> verts[] = {
    array<float,5>(s3, t3, float(x3), float(y3), float(z3)),
    array<float,5>(s2, t2, float(x2), float(y2), float(z2)),
    array<float,5>(s1, t1, float(x1), float(y1), float(z1)),
    array<float,5>(s0, t0, float(x0), float(y0), float(z0))
  };

  ogl::bindtexture(GL_TEXTURE_2D, texture);
  ogl::immvertexsize(sizeof(float[5]));
  ogl::immattrib(ogl::POS0, 3, GL_FLOAT, sizeof(float[2]));
  ogl::immattrib(ogl::TEX0, 2, GL_FLOAT, 0);
  ogl::immdrawelements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, twotriangles, &verts[0][0]);
}

void drawenvbox(int t, int w) {
  OGL(DepthMask, GL_FALSE);
  ogl::bindfixedshader(ogl::DIFFUSETEX);
  ogl::setattribarray()(ogl::POS0, ogl::TEX0);
  drawenvboxface(1.0f, 1.0f, -w, -w,  w,
                 0.0f, 1.0f,  w, -w,  w,
                 0.0f, 0.0f,  w, -w, -w,
                 1.0f, 0.0f, -w, -w, -w, t);
  drawenvboxface(1.0f, 1.0f, +w,  w,  w,
                 0.0f, 1.0f, -w,  w,  w,
                 0.0f, 0.0f, -w,  w, -w,
                 1.0f, 0.0f, +w,  w, -w, t+1);
  drawenvboxface(0.0f, 0.0f, -w, -w, -w,
                 1.0f, 0.0f, -w,  w, -w,
                 1.0f, 1.0f, -w,  w,  w,
                 0.0f, 1.0f, -w, -w,  w, t+2);
  drawenvboxface(1.0f, 1.0f, +w, -w,  w,
                 0.0f, 1.0f, +w,  w,  w,
                 0.0f, 0.0f, +w,  w, -w,
                 1.0f, 0.0f, +w, -w, -w, t+3);
  drawenvboxface(0.0f, 1.0f, -w,  w,  w,
                 0.0f, 0.0f, +w,  w,  w,
                 1.0f, 0.0f, +w, -w,  w,
                 1.0f, 1.0f, -w, -w,  w, t+4);
  drawenvboxface(0.0f, 1.0f, +w,  w, -w,
                 0.0f, 0.0f, -w,  w, -w,
                 1.0f, 0.0f, -w, -w, -w,
                 1.0f, 1.0f, +w, -w, -w, t+5);
  OGL(DepthMask, GL_TRUE);
}
} /* namespace text */
} /* namespace q */

