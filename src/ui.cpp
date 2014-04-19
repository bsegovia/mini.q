/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ui.cpp -> implements immediate mode rendering
 -------------------------------------------------------------------------*/
#include "ui.hpp"
#include "ogl.hpp"
#include "text.hpp"
#include "base/sys.hpp"
#include "base/math.hpp"
#include "base/vector.hpp"

namespace q {
namespace ui {

/*-------------------------------------------------------------------------
 - render interface
 -------------------------------------------------------------------------*/
enum GFX {
  GFX_RECT,
  GFX_TRIANGLE,
  GFX_LINE,
  GFX_TEXT,
  GFX_SCISSOR,
};

struct gfxrect { s16 x,y,w,h,r; };
struct gfxline { s16 x0,y0,x1,y1,r; };
struct gfxtext {
  s16 x,y,align;
  const char *text;
};
struct gfx {
  char type;
  char flags;
  char pad[2];
  u32 col;
  union {
    gfxline line;
    gfxrect rect;
    gfxtext text;
  };
};

/*-------------------------------------------------------------------------
 - graphics commands
 -------------------------------------------------------------------------*/
static const u32 GFX_QUEUE_SIZE = 5000;
static gfx g_gfxqueue[GFX_QUEUE_SIZE];
static u32 g_gfxqueuesize = 0;
static const u32 TEXT_POOL_SIZE = 8000;
static char g_textpool[TEXT_POOL_SIZE];
static u32 g_textpoolsize = 0;

static const gfx *getrenderqueue() { return g_gfxqueue; }
static int getrenderqueuesize() { return g_gfxqueuesize; }

static const char *allocText(const char *text) {
  u32 len = strlen(text)+1;
  if (g_textpoolsize + len >= TEXT_POOL_SIZE) return 0;
  char *dst = &g_textpool[g_textpoolsize];
  memcpy(dst, text, len);
  g_textpoolsize += len;
  return dst;
}

static void resetgfxqueue() {
  g_gfxqueuesize = 0;
  g_textpoolsize = 0;
}

static void addgfxscissor(int x, int y, int w, int h) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_SCISSOR;
  cmd.flags = x < 0 ? 0 : 1;      // on/off flag.
  cmd.col = 0;
  cmd.rect.x = s16(x);
  cmd.rect.y = s16(y);
  cmd.rect.w = s16(w);
  cmd.rect.h = s16(h);
}

static void addgfxrect(float x, float y, float w, float h, u32 col) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_RECT;
  cmd.flags = 0;
  cmd.col = col;
  cmd.rect.x = s16(x*8.0f);
  cmd.rect.y = s16(y*8.0f);
  cmd.rect.w = s16(w*8.0f);
  cmd.rect.h = s16(h*8.0f);
  cmd.rect.r = 0;
}

static void addgfxline(float x0, float y0, float x1, float y1, float r, u32 col) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_LINE;
  cmd.flags = 0;
  cmd.col = col;
  cmd.line.x0 = s16(x0*8.0f);
  cmd.line.y0 = s16(y0*8.0f);
  cmd.line.x1 = s16(x1*8.0f);
  cmd.line.y1 = s16(y1*8.0f);
  cmd.line.r  = s16(r*8.0f);
}

static void addgfxroundedrect(float x, float y, float w, float h, float r, u32 col) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_RECT;
  cmd.flags = 0;
  cmd.col = col;
  cmd.rect.x = s16(x*8.0f);
  cmd.rect.y = s16(y*8.0f);
  cmd.rect.w = s16(w*8.0f);
  cmd.rect.h = s16(h*8.0f);
  cmd.rect.r = s16(r*8.0f);
}

static void addgfxtri(int x, int y, int w, int h, int flags, u32 col) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_TRIANGLE;
  cmd.flags = char(flags);
  cmd.col = col;
  cmd.rect.x = s16(x*8.0f);
  cmd.rect.y = s16(y*8.0f);
  cmd.rect.w = s16(w*8.0f);
  cmd.rect.h = s16(h*8.0f);
}

static void addgfxtext(int x, int y, int align, const char *text, u32 col) {
  if (g_gfxqueuesize >= GFX_QUEUE_SIZE) return;
  gfx& cmd = g_gfxqueue[g_gfxqueuesize++];
  cmd.type = GFX_TEXT;
  cmd.flags = 0;
  cmd.col = col;
  cmd.text.x = s16(x);
  cmd.text.y = s16(y);
  cmd.text.align = s16(align);
  cmd.text.text = allocText(text);
}

/*-------------------------------------------------------------------------
 - OGL code
 -------------------------------------------------------------------------*/
static const u32 TEMP_COORD_COUNT = 100;
static const int CIRCLE_VERTS = 8*4;

// XXX use array struct instead
static float tempcoords[TEMP_COORD_COUNT*2];
static float tempnormals[TEMP_COORD_COUNT*2];
static float circleverts[CIRCLE_VERTS*2];
static bool initcircleverts = true;

static void drawpoly(const float *coords, u32 numCoords, float r, u32 col) {
  if (numCoords > TEMP_COORD_COUNT) numCoords = TEMP_COORD_COUNT;

  for (u32 i = 0, j = numCoords-1; i < numCoords; j=i++) {
    const float* v0 = &coords[j*2];
    const float* v1 = &coords[i*2];
    float dx = v1[0] - v0[0];
    float dy = v1[1] - v0[1];
    float d = sqrtf(dx*dx+dy*dy);
    if (d > 0) {
      d = 1.0f/d;
      dx *= d;
      dy *= d;
    }
    tempnormals[j*2+0] = dy;
    tempnormals[j*2+1] = -dx;
  }

  for (u32 i = 0, j = numCoords-1; i < numCoords; j=i++) {
    float dlx0 = tempnormals[j*2+0];
    float dly0 = tempnormals[j*2+1];
    float dlx1 = tempnormals[i*2+0];
    float dly1 = tempnormals[i*2+1];
    float dmx = (dlx0 + dlx1) * 0.5f;
    float dmy = (dly0 + dly1) * 0.5f;
    float dmr2 = dmx*dmx + dmy*dmy;
    if (dmr2 > 0.000001f) {
      float scale = 1.0f / dmr2;
      if (scale > 10.0f) scale = 10.0f;
      dmx *= scale;
      dmy *= scale;
    }
    tempcoords[i*2+0] = coords[i*2+0]+dmx*r;
    tempcoords[i*2+1] = coords[i*2+1]+dmy*r;
  }

  const auto colTrans = vec4f(float(col&0xff), float((col>>8)&0xff), float((col>>16)&0xff), 0.f) / 255.f;
  const auto colf = vec4f(float(col&0xff), float((col>>8)&0xff), float((col>>16)&0xff), float(col>>24)) / 255.f;

  typedef array<float,6> verttype;
  // XXX remove this and use a static array as the rest
  vector<verttype> verts;
  for (u32 i = 0, j = numCoords-1; i < numCoords; j=i++) {
    verts.add(verttype(colf,coords[i*2+0],coords[i*2+1]));
    verts.add(verttype(colf,coords[j*2+0],coords[j*2+1]));
    verts.add(verttype(colTrans,tempcoords[j*2+0],tempcoords[j*2+1]));
    verts.add(verttype(colTrans,tempcoords[j*2+0],tempcoords[j*2+1]));
    verts.add(verttype(colTrans,tempcoords[i*2+0],tempcoords[i*2+1]));
    verts.add(verttype(colf,coords[i*2+0],coords[i*2+1]));
  }
  for (u32 i = 2; i < numCoords; ++i) {
    verts.add(verttype(colf, coords[0], coords[1]));
    verts.add(verttype(colf, coords[(i-1)*2], coords[(i-1)*2+1]));
    verts.add(verttype(colf, coords[i*2], coords[i*2+1]));
  }
  ogl::bindfixedshader(ogl::FIXED_COLOR);
  ogl::immdraw("Tc4p2", verts.length(), &verts[0][0]);
}

static void drawrect(float x, float y, float w, float h, float fth, u32 col) {
  const float verts[4*2] = {
    x+0.5f, y+0.5f,
    x+w-0.5f, y+0.5f,
    x+w-0.5f, y+h-0.5f,
    x+0.5f, y+h-0.5f,
  };
  drawpoly(verts, 4, fth, col);
}

static void drawroundedrect(float x, float y, float w, float h, float r, float fth, u32 col) {
  printf("BOUM %f %f %f %f %f %f\n", x, y, w, h, r, fth);
  const u32 n = CIRCLE_VERTS/4;
  float verts[(n+1)*4*2];
  const float *cverts = circleverts;
  float *v = verts;
  if (initcircleverts) {
    loopi(CIRCLE_VERTS) {
      float a = float(i)/float(CIRCLE_VERTS)*float(pi)*2.f;
      circleverts[i*2+0] = cos(a);
      circleverts[i*2+1] = sin(a);
    }
    initcircleverts = false;
  }

  for (u32 i = 0; i <= n; ++i) {
    *v++ = x+w-r + cverts[i*2]*r;
    *v++ = y+h-r + cverts[i*2+1]*r;
  }
  for (u32 i = n; i <= n*2; ++i) {
    *v++ = x+r + cverts[i*2]*r;
    *v++ = y+h-r + cverts[i*2+1]*r;
  }
  for (u32 i = n*2; i <= n*3; ++i) {
    *v++ = x+r + cverts[i*2]*r;
    *v++ = y+r + cverts[i*2+1]*r;
  }
  for (u32 i = n*3; i < n*4; ++i) {
    *v++ = x+w-r + cverts[i*2]*r;
    *v++ = y+r + cverts[i*2+1]*r;
  }
  *v++ = x+w-r + cverts[0]*r;
  *v++ = y+r + cverts[1]*r;

  drawpoly(verts, (n+1)*4, fth, col);
}

static void drawline(float x0, float y0, float x1, float y1, float r, float fth, u32 col) {
  float dx = x1-x0;
  float dy = y1-y0;
  float d = sqrtf(dx*dx+dy*dy);
  if (d > 0.0001f) {
    d = 1.0f/d;
    dx *= d;
    dy *= d;
  }
  float nx = dy;
  float ny = -dx;
  float verts[4*2];
  r -= fth;
  r *= 0.5f;
  if (r < 0.01f) r = 0.01f;
  dx *= r;
  dy *= r;
  nx *= r;
  ny *= r;

  verts[0] = x0-dx-nx;
  verts[1] = y0-dy-ny;
  verts[2] = x0-dx+nx;
  verts[3] = y0-dy+ny;
  verts[4] = x1+dx+nx;
  verts[5] = y1+dy+ny;
  verts[6] = x1+dx-nx;
  verts[7] = y1+dy-ny;

  drawpoly(verts, 4, fth, col);
}

static void drawtext(float x, float y, const char *text, int align, u32 c) {
  if (align == ALIGN_CENTER)
    x -= int(text::width(text))/2;
  else if (align == ALIGN_RIGHT)
    x -= int(text::width(text));
  text::drawf(text, vec2f(x,y));
}

void draw(int width, int height) {
  const auto q = getrenderqueue();
  const auto nq = getrenderqueuesize();
  const auto s = 1.0f/8.0f;

  OGL(Disable, GL_SCISSOR_TEST);
  for (int i = 0; i < nq; ++i) {
    const gfx& cmd = q[i];
    if (cmd.type == GFX_RECT) {
      if (cmd.rect.r == 0)
        drawrect((float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
                 (float)cmd.rect.w*s-1.f, (float)cmd.rect.h*s-1.f,
                 1.0f, cmd.col);
      else
        drawroundedrect((float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
                (float)cmd.rect.w*s-1, (float)cmd.rect.h*s-1,
                (float)cmd.rect.r*s, 1.0f, cmd.col);
    }
    else if (cmd.type == GFX_LINE)
      drawline(cmd.line.x0*s, cmd.line.y0*s, cmd.line.x1*s, cmd.line.y1*s, cmd.line.r*s, 1.0f, cmd.col);
    else if (cmd.type == GFX_TRIANGLE) {
      if (cmd.flags == 1) {
        const float verts[3*2] = {
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s-1, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s/2-0.5f,
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
        };
        drawpoly(verts, 3, 1.0f, cmd.col);
      }
      if (cmd.flags == 2) {
        const float verts[3*2] = {
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s/2-0.5f, (float)cmd.rect.y*s+0.5f,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s-1, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
        };
        drawpoly(verts, 3, 1.0f, cmd.col);
      }
    } else if (cmd.type == GFX_TEXT)
      drawtext(cmd.text.x, cmd.text.y, cmd.text.text, cmd.text.align, cmd.col);
    else if (cmd.type == GFX_SCISSOR) {
      if (cmd.flags) {
        OGL(Enable, GL_SCISSOR_TEST);
        OGL(Scissor, cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h);
      } else
        OGL(Disable, GL_SCISSOR_TEST);
    }
  }
  OGL(Disable, GL_SCISSOR_TEST);
}

/*-------------------------------------------------------------------------
 - user interface
 -------------------------------------------------------------------------*/
struct guistate {
  guistate() :
    left(false), leftPressed(false), leftReleased(false),
    mx(-1), my(-1), scroll(0),
    active(0), hot(0), hotToBe(0), isHot(false), isActive(false), wentActive(false),
    dragX(0), dragY(0), dragOrig(0), widgetX(0), widgetY(0), widgetW(100),
    insideCurrentScroll(false),  areaId(0), widgetId(0)
  {}

  bool left;
  bool leftPressed, leftReleased;
  int mx,my;
  int scroll;
  u32 active;
  u32 hot;
  u32 hotToBe;
  bool isHot;
  bool isActive;
  bool wentActive;
  int dragX, dragY;
  float dragOrig;
  int widgetX, widgetY, widgetW;
  bool insideCurrentScroll;

  u32 areaId;
  u32 widgetId;
};

static guistate state;

INLINE bool anyActive() { return state.active != 0; }
INLINE bool isActive(u32 id) { return state.active == id; }
INLINE bool isHot(u32 id) { return state.hot == id; }
INLINE bool inrect(int x, int y, int w, int h, bool checkScroll = true) {
  return (!checkScroll || state.insideCurrentScroll) &&
         state.mx >= x && state.mx <= x+w &&
         state.my >= y && state.my <= y+h;
}
INLINE void clearInput() {
  state.leftPressed = false;
  state.leftReleased = false;
  state.scroll = 0;
}
INLINE void clearActive() {
  state.active = 0;
  clearInput();
}
INLINE void setActive(u32 id) {
  state.active = id;
  state.wentActive = true;
}
INLINE void setHot(u32 id) { state.hotToBe = id; }

static bool buttonLogic(u32 id, bool over) {
  bool res = false;
  // process down
  if (!anyActive()) {
    if (over) setHot(id);
    if (isHot(id) && state.leftPressed) setActive(id);
  }

  // if button is active, then react on left up
  if (isActive(id)) {
    state.isActive = true;
    if (over) setHot(id);
    if (state.leftReleased) {
      if (isHot(id))
        res = true;
      clearActive();
    }
  }

  if (isHot(id)) state.isHot = true;
  return res;
}

static void updateInput(int mx, int my, u8 mbut, int scroll) {
  bool left = (mbut & MBUT_LEFT) != 0;
  state.mx = mx;
  state.my = my;
  state.leftPressed = !state.left && left;
  state.leftReleased = state.left && !left;
  state.left = left;
  state.scroll = scroll;
}

void beginframe(int mx, int my, u8 mbut, int scroll) {
  updateInput(mx,my,mbut,scroll);
  state.hot = state.hotToBe;
  state.hotToBe = 0;
  state.wentActive = false;
  state.isActive = false;
  state.isHot = false;
  state.widgetX = 0;
  state.widgetY = 0;
  state.widgetW = 0;
  state.areaId = 1;
  state.widgetId = 1;
  resetgfxqueue();
}

void endframe() { clearInput(); }

static const int BUTTON_HEIGHT = 20;
static const int SLIDER_HEIGHT = 20;
static const int SLIDER_MARKER_WIDTH = 10;
static const int CHECK_SIZE = 8;
static const int DEFAULT_SPACING = 4;
static const int TEXT_HEIGHT = 16;
static const int SCROLL_AREA_PADDING = 6;
static const int INDENT_SIZE = 16;
static const int AREA_HEADER = 28;

static int g_scrollTop = 0;
static int g_scrollBottom = 0;
static int g_scrollRight = 0;
static int g_scrollAreaTop = 0;
static int *g_scrollVal = 0;
static int g_focusTop = 0;
static int g_focusBottom = 0;
static u32 g_scrollId = 0;
static bool g_insideScrollArea = false;

bool beginscrollarea(const char *name, int x, int y, int w, int h, int *scroll) {
  state.areaId++;
  state.widgetId = 0;
  g_scrollId = (state.areaId<<16) | state.widgetId;

  state.widgetX = x + SCROLL_AREA_PADDING;
  state.widgetY = y+h-AREA_HEADER + (*scroll);
  state.widgetW = w - SCROLL_AREA_PADDING*4;
  g_scrollTop = y-AREA_HEADER+h;
  g_scrollBottom = y+SCROLL_AREA_PADDING;
  g_scrollRight = x+w - SCROLL_AREA_PADDING*3;
  g_scrollVal = scroll;

  g_scrollAreaTop = state.widgetY;

  g_focusTop = y-AREA_HEADER;
  g_focusBottom = y-AREA_HEADER+h;

  g_insideScrollArea = inrect(x, y, w, h, false);
  state.insideCurrentScroll = g_insideScrollArea;

  addgfxroundedrect(float(x), float(y), float(w), float(h), 6, rgba(0,0,0,192));
  addgfxtext(x+AREA_HEADER/2, y+h-AREA_HEADER/2-TEXT_HEIGHT/2, ALIGN_LEFT, name, rgba(255,255,255,128));
  addgfxscissor(x+SCROLL_AREA_PADDING, y+SCROLL_AREA_PADDING, w-SCROLL_AREA_PADDING*4, h-AREA_HEADER-SCROLL_AREA_PADDING);

  return g_insideScrollArea;
}

void endscrollarea() {
  // Disable scissoring.
  addgfxscissor(-1,-1,-1,-1);

  // Draw scroll bar
  int x = g_scrollRight+SCROLL_AREA_PADDING/2;
  int y = g_scrollBottom;
  int w = SCROLL_AREA_PADDING*2;
  int h = g_scrollTop - g_scrollBottom;

  int stop = g_scrollAreaTop;
  int sbot = state.widgetY;
  int sh = stop - sbot; // The scrollable area height.

  float barHeight = (float)h/(float)sh;

  if (barHeight < 1) {
    float barY = (float)(y - sbot)/(float)sh;
    if (barY < 0) barY = 0;
    if (barY > 1) barY = 1;

    // Handle scroll bar logic.
    u32 hid = g_scrollId;
    int hx = x;
    int hy = y + (int)(barY*h);
    int hw = w;
    int hh = (int)(barHeight*h);

    const int range = h - (hh-1);
    bool over = inrect(hx, hy, hw, hh);
    buttonLogic(hid, over);
    if (isActive(hid))
    {
      float u = (float)(hy-y) / (float)range;
      if (state.wentActive)
      {
        state.dragY = state.my;
        state.dragOrig = u;
      }
      if (state.dragY != state.my)
      {
        u = state.dragOrig + (state.my - state.dragY) / (float)range;
        if (u < 0) u = 0;
        if (u > 1) u = 1;
        *g_scrollVal = (int)((1-u)  *(sh - h));
      }
    }

    // BG
    addgfxroundedrect(float(x), float(y), float(w), float(h), float(w)/2-1, rgba(0,0,0,196));
    // Bar
    if (isActive(hid))
      addgfxroundedrect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, rgba(255,196,0,196));
    else
      addgfxroundedrect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, isHot(hid) ? rgba(255,196,0,96) : rgba(255,255,255,64));

    // Handle mouse scrolling.
    if (g_insideScrollArea) { // && !anyActive())
      if (state.scroll) {
        *g_scrollVal += 20*state.scroll;
        if (*g_scrollVal < 0) *g_scrollVal = 0;
        if (*g_scrollVal > (sh - h)) *g_scrollVal = (sh - h);
      }
    }
  }
  state.insideCurrentScroll = false;
}

bool button(const char *text, bool enabled) {
  state.widgetId++;
  u32 id = (state.areaId<<16) | state.widgetId;

  const int x = state.widgetX;
  const int y = state.widgetY - BUTTON_HEIGHT;
  const int w = state.widgetW;
  const int h = BUTTON_HEIGHT;
  state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  const bool over = enabled && inrect(x, y, w, h);
  const bool res = buttonLogic(id, over);

  addgfxroundedrect(float(x), float(y), float(w), float(h), (float)BUTTON_HEIGHT/2-1, rgba(128,128,128, isActive(id)?196:96));
  if (enabled)
    addgfxtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool item(const char *text, bool enabled) {
  state.widgetId++;
  u32 id = (state.areaId<<16) | state.widgetId;

  int x = state.widgetX;
  int y = state.widgetY - BUTTON_HEIGHT;
  int w = state.widgetW;
  int h = BUTTON_HEIGHT;
  state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  if (isHot(id))
    addgfxroundedrect(float(x), float(y), float(w), float(h), 2.0f, rgba(255,196,0,isActive(id)?196:96));
  if (enabled)
    addgfxtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(255,255,255,200));
  else
    addgfxtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool check(const char *text, bool checked, bool enabled) {
  state.widgetId++;
  u32 id = (state.areaId<<16) | state.widgetId;

  int x = state.widgetX;
  int y = state.widgetY - BUTTON_HEIGHT;
  int w = state.widgetW;
  int h = BUTTON_HEIGHT;
  state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  addgfxroundedrect((float)cx-3, (float)cy-3, (float)CHECK_SIZE+6, (float)CHECK_SIZE+6, 4, rgba(128,128,128, isActive(id)?196:96));
  if (checked) {
    if (enabled)
      addgfxroundedrect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, rgba(255,255,255,isActive(id)?255:200));
    else
      addgfxroundedrect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, rgba(128,128,128,200));
  }

  if (enabled)
    addgfxtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool collapse(const char *text, const char *subtext, bool checked, bool enabled) {
  state.widgetId++;
  u32 id = (state.areaId<<16) | state.widgetId;

  int x = state.widgetX;
  int y = state.widgetY - BUTTON_HEIGHT;
  int w = state.widgetW;
  int h = BUTTON_HEIGHT;
  state.widgetY -= BUTTON_HEIGHT; // + DEFAULT_SPACING;

  const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  if (checked)
    addgfxtri(cx, cy, CHECK_SIZE, CHECK_SIZE, 2, rgba(255,255,255,isActive(id)?255:200));
  else
    addgfxtri(cx, cy, CHECK_SIZE, CHECK_SIZE, 1, rgba(255,255,255,isActive(id)?255:200));

  if (enabled)
    addgfxtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  if (subtext)
    addgfxtext(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, subtext, rgba(255,255,255,128));

  return res;
}

void label(const char *text) {
  int x = state.widgetX;
  int y = state.widgetY - BUTTON_HEIGHT;
  state.widgetY -= BUTTON_HEIGHT;
  addgfxtext(x, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(255,255,255,255));
}

void value(const char *text) {
  const int x = state.widgetX;
  const int y = state.widgetY - BUTTON_HEIGHT;
  const int w = state.widgetW;
  state.widgetY -= BUTTON_HEIGHT;

  addgfxtext(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, text, rgba(255,255,255,200));
}

bool slider(const char *text, float *val, float vmin, float vmax, float vinc, bool enabled) {
  state.widgetId++;
  u32 id = (state.areaId<<16) | state.widgetId;

  int x = state.widgetX;
  int y = state.widgetY - BUTTON_HEIGHT;
  int w = state.widgetW;
  int h = SLIDER_HEIGHT;
  state.widgetY -= SLIDER_HEIGHT + DEFAULT_SPACING;

  addgfxroundedrect(float(x), float(y), float(w), float(h), 4.0f, rgba(0,0,0,128));

  const int range = w - SLIDER_MARKER_WIDTH;

  float u = (*val - vmin) / (vmax-vmin);
  if (u < 0) u = 0;
  if (u > 1) u = 1;
  int m = int(u*range);

  bool over = enabled && inrect(x+m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT);
  bool res = buttonLogic(id, over);
  bool valChanged = false;

  if (isActive(id)) {
    if (state.wentActive) {
      state.dragX = state.mx;
      state.dragOrig = u;
    }
    if (state.dragX != state.mx) {
      u = state.dragOrig + float(state.mx - state.dragX) / float(range);
      if (u < 0) u = 0;
      if (u > 1) u = 1;
      *val = vmin + u*(vmax-vmin);
      *val = floorf(*val/vinc+0.5f)*vinc; // Snap to vinc
      m = (int)(u  *range);
      valChanged = true;
    }
  }

  if (isActive(id))
    addgfxroundedrect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, rgba(255,255,255,255));
  else
    addgfxroundedrect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, isHot(id) ? rgba(255,196,0,128) : rgba(255,255,255,64));

  // TODO: fix this, take a look at 'nicenum'.
  const int digits = (int)(ceilf(log10f(vinc)));
  char fmt[16], msg[128];
  snprintf(fmt, 16, "%%.%df", digits >= 0 ? 0 : -digits);
  snprintf(msg, 128, fmt, *val);

  if (enabled) {
    addgfxtext(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
    addgfxtext(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, msg, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  } else {
    addgfxtext(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));
    addgfxtext(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, msg, rgba(128,128,128,200));
  }

  return res || valChanged;
}

void indent() {
  state.widgetX += INDENT_SIZE;
  state.widgetW -= INDENT_SIZE;
}

void unindent() {
  state.widgetX -= INDENT_SIZE;
  state.widgetW += INDENT_SIZE;
}

void separator() { state.widgetY -= DEFAULT_SPACING*3; }
void separatorline() {
  int x = state.widgetX;
  int y = state.widgetY - DEFAULT_SPACING*2;
  int w = state.widgetW;
  int h = 1;
  state.widgetY -= DEFAULT_SPACING*4;

  addgfxrect(float(x), float(y), float(w), float(h), rgba(255,255,255,32));
}
void drawtext(int x, int y, int align, const char *text, u32 color) {
  addgfxtext(x, y, align, text, color);
}
void drawline(float x0, float y0, float x1, float y1, float r, u32 color) {
  addgfxline(x0, y0, x1, y1, r, color);
}
void drawrect(float x, float y, float w, float h, u32 color) {
  addgfxrect(x, y, w, h, color);
}
void drawroundedrect(float x, float y, float w, float h, float r, u32 color) {
  addgfxroundedrect(x, y, w, h, r, color);
}
} /* namespace ui */
} /* namespace q */

