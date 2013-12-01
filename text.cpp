/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - text.cpp -> implements text rendering routines
 -------------------------------------------------------------------------*/
#include "ogl.hpp"
#include "text.hpp"
#include "renderer.hpp"
#include "math.hpp"

namespace q {
namespace text {

static const short char_coords[96][4] = {
  {0,0,25,64},        //!
  {25,0,54,64},       //"
  {54,0,107,64},      //#
  {107,0,148,64},     //$
  {148,0,217,64},     //%
  {217,0,263,64},     //&
  {263,0,280,64},     //'
  {280,0,309,64},     //(
  {309,0,338,64},     //)
  {338,0,379,64},     //*
  {379,0,432,64},     //+
  {432,0,455,64},     //,
  {455,0,484,64},     //-
  {0,64,21,128},      //.
  {23,64,52,128},     ///
  {52,64,93,128},     //0
  {93,64,133,128},    //1
  {133,64,174,128},   //2
  {174,64,215,128},   //3
  {215,64,256,128},   //4
  {256,64,296,128},   //5
  {296,64,337,128},   //6
  {337,64,378,128},   //7
  {378,64,419,128},   //8
  {419,64,459,128},   //9
  {459,64,488,128},   //:
  {0,128,29,192},     //;
  {29,128,81,192},    //<
  {81,128,134,192},   //=
  {134,128,186,192},  //>
  {186,128,221,192},  //?
  {221,128,285,192},  //@
  {285,128,329,192},  //A
  {329,128,373,192},  //B
  {373,128,418,192},  //C
  {418,128,467,192},  //D
  {0,192,40,256},     //E
  {40,192,77,256},    //F
  {77,192,127,256},   //G
  {127,192,175,256},  //H
  {175,192,202,256},  //I
  {202,192,231,256},  //J
  {231,192,275,256},  //K
  {275,192,311,256},  //L
  {311,192,365,256},  //M
  {365,192,413,256},  //N
  {413,192,463,256},  //O
  {1,256,38,320},     //P
  {38,256,89,320},    //Q
  {89,256,133,320},   //R
  {133,256,176,320},  //S
  {177,256,216,320},  //T
  {217,256,263,320},  //U
  {263,256,307,320},  //V
  {307,256,370,320},  //W
  {370,256,414,320},  //X
  {414,256,453,320},  //Y
  {453,256,497,320},  //Z
  {0,320,29,384},     //[
  {29,320,58,384},    //"\"
  {59,320,87,384},    //]
  {87,320,139,384},   //^
  {139,320,180,384},  //_
  {180,320,221,384},  //`
  {221,320,259,384},  //a
  {259,320,299,384},  //b
  {299,320,332,384},  //c
  {332,320,372,384},  //d
  {372,320,411,384},  //e
  {411,320,433,384},  //f
  {435,320,473,384},  //g
  {0,384,40,448},     //h
  {40,384,56,448},    //i
  {58,384,80,448},    //j
  {80,384,118,448},   //k
  {118,384,135,448},  //l
  {135,384,197,448},  //m
  {197,384,238,448},  //n
  {238,384,277,448},  //o
  {277,384,317,448},  //p
  {317,384,356,448},  //q
  {357,384,384,448},  //r
  {385,384,417,448},  //s
  {417,384,442,448},  //t
  {443,384,483,448},  //u
  {0,448,38,512},     //v
  {38,448,90,512},    //w
  {90,448,128,512},   //x
  {128,448,166,512},  //y
  {166,448,200,512},  //z
  {200,448,241,512},  //{
  {241,448,270,512},  //|
  {270,448,310,512},  //}
  {310,448,363,512}   //~
};

typedef u16 indextype;
static const indextype twotriangles[] = {0,1,2,0,2,3};

int width(const char *str) {
  int x = 0;
  for (int i = 0; str[i] != 0; i++) {
    int c = str[i];
    if (c=='\t') { x = (x+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB; continue; }; 
    if (c=='\f') continue; 
    if (c==' ') { x += rr::FONTH/2; continue; };
    c -= 33;
    if (c<0 || c>=95) continue;
    const int in_width = char_coords[c][2] - char_coords[c][0];
    x += in_width + 1;
  }
  return x;
}

void drawf(const char *fstr, int left, int top, ...) {
  sprintf_sdlv(str, top, fstr);
  draw(str, left, top);
}

void draw(const char *str, int left, int top) {
  typedef array<float,4> arrayf4;
  OGL(BlendFunc, GL_ONE, GL_ONE);
  ogl::bindtexture(GL_TEXTURE_2D, ogl::coretex(ogl::TEX_CHARACTERS));
  OGL(VertexAttrib3f,ogl::COL,1.f,1.f,1.f);

  // use a triangle mesh to display the text
  const size_t len = strlen(str);
  indextype *indices = (indextype*) alloca(6*len*sizeof(int));
  array<float,4> *verts = (array<float,4>*) alloca(4*len*sizeof(array<float,4>));

  // traverse the string and build the mesh
  int index = 0, vert = 0, x = left, y = top;
  for (int i = 0; str[i] !=	 0; ++i) {
    int c = str[i];
    if (c=='\t') { x = (x-left+rr::PIXELTAB)/rr::PIXELTAB*rr::PIXELTAB+left; continue; }; 
    if (c=='\f') { OGL(VertexAttrib3f,ogl::COL,0.25f,1.f,0.5f); continue; };
    if (c==' ') { x += rr::FONTH/2; continue; };
    c -= 33;
    if (c<0 || c>=95) continue;

    const float in_left   = float(char_coords[c][0])   / 512.0f;
    const float in_top    = float(char_coords[c][1]+2) / 512.0f;
    const float in_right  = float(char_coords[c][2])   / 512.0f;
    const float in_bottom = float(char_coords[c][3]-2) / 512.0f;
    const int in_width  = char_coords[c][2] - char_coords[c][0];
    const int in_height = char_coords[c][3] - char_coords[c][1];

    loopj(6) indices[index+j] = vert+twotriangles[j];
    verts[vert+0] = arrayf4(in_left, in_top,   float(x),         float(y));
    verts[vert+1] = arrayf4(in_right,in_top,   float(x+in_width),float(y));
    verts[vert+2] = arrayf4(in_right,in_bottom,float(x+in_width),float(y+in_height));
    verts[vert+3] = arrayf4(in_left, in_bottom,float(x),         float(y+in_height));

    x += in_width + 1;
    index += 6;
    vert += 4;
  }

  ogl::setattribarray()(ogl::POS0, ogl::TEX0);
  ogl::immvertexsize(sizeof(float[4]));
  ogl::immattrib(ogl::POS0, 2, GL_FLOAT, sizeof(float[2]));
  ogl::immattrib(ogl::TEX0, 2, GL_FLOAT, 0);
  ogl::fixedshader(ogl::DIFFUSETEX);
  ogl::immdrawelements(GL_TRIANGLES, index, GL_UNSIGNED_SHORT, indices, &verts[0][0]);
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
  ogl::fixedshader(ogl::DIFFUSETEX);
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

