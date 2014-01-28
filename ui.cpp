/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ui.cpp -> implements immediate mode rendering
 -------------------------------------------------------------------------*/
#include "ui.hpp"
#include "sys.hpp"
#include "stl.hpp"
#include "ogl.hpp"
#include "math.hpp"
#if 1
#include <GL/gl.h>

namespace q {
namespace ui {

/*-------------------------------------------------------------------------
 - pull render interface
 -------------------------------------------------------------------------*/
enum GfxCmdType {
  GFXCMD_RECT,
  GFXCMD_TRIANGLE,
  GFXCMD_LINE,
  GFXCMD_TEXT,
  GFXCMD_SCISSOR,
};

struct gfxrect {
  s16 x,y,w,h,r;
};

struct gfxtext {
  s16 x,y,align;
  const char *text;
};

struct gfxline {
  s16 x0,y0,x1,y1,r;
};

struct gfxcmd {
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

const gfxcmd *getrenderqueue();
int getrenderqueuesize();

/*-------------------------------------------------------------------------
 - graphics commands
 -------------------------------------------------------------------------*/
static const unsigned TEXT_POOL_SIZE = 8000;
static char g_textpool[TEXT_POOL_SIZE];
static unsigned g_textpoolsize = 0;

static const char *allocText(const char *text) {
  unsigned len = strlen(text)+1;
  if (g_textpoolsize + len >= TEXT_POOL_SIZE) return 0;
  char *dst = &g_textpool[g_textpoolsize];
  memcpy(dst, text, len);
  g_textpoolsize += len;
  return dst;
}

static const unsigned GFXCMD_QUEUE_SIZE = 5000;
static gfxcmd g_gfxcmdqueue[GFXCMD_QUEUE_SIZE];
static unsigned g_gfxcmdqueuesize = 0;

static void resetgfxcmdqueue() {
  g_gfxcmdqueuesize = 0;
  g_textpoolsize = 0;
}

static void addgfxcmdscissor(int x, int y, int w, int h) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_SCISSOR;
  cmd.flags = x < 0 ? 0 : 1;      // on/off flag.
  cmd.col = 0;
  cmd.rect.x = (short)x;
  cmd.rect.y = (short)y;
  cmd.rect.w = (short)w;
  cmd.rect.h = (short)h;
}

static void addgfxcmdrect(float x, float y, float w, float h, u32 color) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_RECT;
  cmd.flags = 0;
  cmd.col = color;
  cmd.rect.x = (short)(x*8.0f);
  cmd.rect.y = (short)(y*8.0f);
  cmd.rect.w = (short)(w*8.0f);
  cmd.rect.h = (short)(h*8.0f);
  cmd.rect.r = 0;
}

static void addgfxcmdline(float x0, float y0, float x1, float y1, float r, u32 color) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_LINE;
  cmd.flags = 0;
  cmd.col = color;
  cmd.line.x0 = (short)(x0*8.0f);
  cmd.line.y0 = (short)(y0*8.0f);
  cmd.line.x1 = (short)(x1*8.0f);
  cmd.line.y1 = (short)(y1*8.0f);
  cmd.line.r = (short)(r*8.0f);
}

static void addgfxcmdroundedrect(float x, float y, float w, float h, float r, u32 color) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_RECT;
  cmd.flags = 0;
  cmd.col = color;
  cmd.rect.x = (short)(x*8.0f);
  cmd.rect.y = (short)(y*8.0f);
  cmd.rect.w = (short)(w*8.0f);
  cmd.rect.h = (short)(h*8.0f);
  cmd.rect.r = (short)(r*8.0f);
}

static void addgfxcmdTriangle(int x, int y, int w, int h, int flags, u32 color) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_TRIANGLE;
  cmd.flags = (char)flags;
  cmd.col = color;
  cmd.rect.x = (short)(x*8.0f);
  cmd.rect.y = (short)(y*8.0f);
  cmd.rect.w = (short)(w*8.0f);
  cmd.rect.h = (short)(h*8.0f);
}

static void addgfxcmdtext(int x, int y, int align, const char *text, u32 color) {
  if (g_gfxcmdqueuesize >= GFXCMD_QUEUE_SIZE) return;
  gfxcmd& cmd = g_gfxcmdqueue[g_gfxcmdqueuesize++];
  cmd.type = GFXCMD_TEXT;
  cmd.flags = 0;
  cmd.col = color;
  cmd.text.x = (short)x;
  cmd.text.y = (short)y;
  cmd.text.align = (short)align;
  cmd.text.text = allocText(text);
}

/*-------------------------------------------------------------------------
 - user interface
 -------------------------------------------------------------------------*/
struct GuiState {
  GuiState() :
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

static GuiState g_state;

INLINE bool anyActive() { return g_state.active != 0; }
INLINE bool isActive(u32 id) { return g_state.active == id; }
INLINE bool isHot(u32 id) { return g_state.hot == id; }
INLINE bool inrect(int x, int y, int w, int h, bool checkScroll = true) {
  return (!checkScroll || g_state.insideCurrentScroll) &&
         g_state.mx >= x && g_state.mx <= x+w &&
         g_state.my >= y && g_state.my <= y+h;
}
INLINE void clearInput() {
  g_state.leftPressed = false;
  g_state.leftReleased = false;
  g_state.scroll = 0;
}
INLINE void clearActive() {
  g_state.active = 0;
  clearInput();
}
INLINE void setActive(u32 id) {
  g_state.active = id;
  g_state.wentActive = true;
}
INLINE void setHot(u32 id) { g_state.hotToBe = id; }


static bool buttonLogic(u32 id, bool over) {
  bool res = false;
  // process down
  if (!anyActive()) {
    if (over)
      setHot(id);
    if (isHot(id) && g_state.leftPressed)
      setActive(id);
  }

  // if button is active, then react on left up
  if (isActive(id)) {
    g_state.isActive = true;
    if (over)
      setHot(id);
    if (g_state.leftReleased) {
      if (isHot(id))
        res = true;
      clearActive();
    }
  }

  if (isHot(id))
    g_state.isHot = true;

  return res;
}

static void updateInput(int mx, int my, unsigned char mbut, int scroll) {
  bool left = (mbut & MBUT_LEFT) != 0;
  g_state.mx = mx;
  g_state.my = my;
  g_state.leftPressed = !g_state.left && left;
  g_state.leftReleased = g_state.left && !left;
  g_state.left = left;
  g_state.scroll = scroll;
}

void beginframe(int mx, int my, unsigned char mbut, int scroll) {
  updateInput(mx,my,mbut,scroll);

  g_state.hot = g_state.hotToBe;
  g_state.hotToBe = 0;

  g_state.wentActive = false;
  g_state.isActive = false;
  g_state.isHot = false;

  g_state.widgetX = 0;
  g_state.widgetY = 0;
  g_state.widgetW = 0;

  g_state.areaId = 1;
  g_state.widgetId = 1;

  resetgfxcmdqueue();
}

void endframe() { clearInput(); }
const gfxcmd *getrenderqueue() { return g_gfxcmdqueue; }
int getrenderqueuesize() { return g_gfxcmdqueuesize; }

static const int BUTTON_HEIGHT = 20;
static const int SLIDER_HEIGHT = 20;
static const int SLIDER_MARKER_WIDTH = 10;
static const int CHECK_SIZE = 8;
static const int DEFAULT_SPACING = 4;
static const int TEXT_HEIGHT = 8;
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
  g_state.areaId++;
  g_state.widgetId = 0;
  g_scrollId = (g_state.areaId<<16) | g_state.widgetId;

  g_state.widgetX = x + SCROLL_AREA_PADDING;
  g_state.widgetY = y+h-AREA_HEADER + (*scroll);
  g_state.widgetW = w - SCROLL_AREA_PADDING*4;
  g_scrollTop = y-AREA_HEADER+h;
  g_scrollBottom = y+SCROLL_AREA_PADDING;
  g_scrollRight = x+w - SCROLL_AREA_PADDING*3;
  g_scrollVal = scroll;

  g_scrollAreaTop = g_state.widgetY;

  g_focusTop = y-AREA_HEADER;
  g_focusBottom = y-AREA_HEADER+h;

  g_insideScrollArea = inrect(x, y, w, h, false);
  g_state.insideCurrentScroll = g_insideScrollArea;

  addgfxcmdroundedrect((float)x, (float)y, (float)w, (float)h, 6, rgba(0,0,0,192));

  addgfxcmdtext(x+AREA_HEADER/2, y+h-AREA_HEADER/2-TEXT_HEIGHT/2, ALIGN_LEFT, name, rgba(255,255,255,128));

  addgfxcmdscissor(x+SCROLL_AREA_PADDING, y+SCROLL_AREA_PADDING, w-SCROLL_AREA_PADDING*4, h-AREA_HEADER-SCROLL_AREA_PADDING);

  return g_insideScrollArea;
}

void endscrollarea() {
  // Disable scissoring.
  addgfxcmdscissor(-1,-1,-1,-1);

  // Draw scroll bar
  int x = g_scrollRight+SCROLL_AREA_PADDING/2;
  int y = g_scrollBottom;
  int w = SCROLL_AREA_PADDING*2;
  int h = g_scrollTop - g_scrollBottom;

  int stop = g_scrollAreaTop;
  int sbot = g_state.widgetY;
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
      if (g_state.wentActive)
      {
        g_state.dragY = g_state.my;
        g_state.dragOrig = u;
      }
      if (g_state.dragY != g_state.my)
      {
        u = g_state.dragOrig + (g_state.my - g_state.dragY) / (float)range;
        if (u < 0) u = 0;
        if (u > 1) u = 1;
        *g_scrollVal = (int)((1-u)  *(sh - h));
      }
    }

    // BG
    addgfxcmdroundedrect((float)x, (float)y, (float)w, (float)h, (float)w/2-1, rgba(0,0,0,196));
    // Bar
    if (isActive(hid))
      addgfxcmdroundedrect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, rgba(255,196,0,196));
    else
      addgfxcmdroundedrect((float)hx, (float)hy, (float)hw, (float)hh, (float)w/2-1, isHot(hid) ? rgba(255,196,0,96) : rgba(255,255,255,64));

    // Handle mouse scrolling.
    if (g_insideScrollArea) { // && !anyActive())
      if (g_state.scroll) {
        *g_scrollVal += 20*g_state.scroll;
        if (*g_scrollVal < 0) *g_scrollVal = 0;
        if (*g_scrollVal > (sh - h)) *g_scrollVal = (sh - h);
      }
    }
  }
  g_state.insideCurrentScroll = false;
}

bool button(const char *text, bool enabled) {
  g_state.widgetId++;
  u32 id = (g_state.areaId<<16) | g_state.widgetId;

  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  int w = g_state.widgetW;
  int h = BUTTON_HEIGHT;
  g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  addgfxcmdroundedrect((float)x, (float)y, (float)w, (float)h, (float)BUTTON_HEIGHT/2-1, rgba(128,128,128, isActive(id)?196:96));
  if (enabled)
    addgfxcmdtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxcmdtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool item(const char *text, bool enabled) {
  g_state.widgetId++;
  u32 id = (g_state.areaId<<16) | g_state.widgetId;

  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  int w = g_state.widgetW;
  int h = BUTTON_HEIGHT;
  g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  if (isHot(id))
    addgfxcmdroundedrect((float)x, (float)y, (float)w, (float)h, 2.0f, rgba(255,196,0,isActive(id)?196:96));

  if (enabled)
    addgfxcmdtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(255,255,255,200));
  else
    addgfxcmdtext(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool check(const char *text, bool checked, bool enabled) {
  g_state.widgetId++;
  u32 id = (g_state.areaId<<16) | g_state.widgetId;

  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  int w = g_state.widgetW;
  int h = BUTTON_HEIGHT;
  g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  addgfxcmdroundedrect((float)cx-3, (float)cy-3, (float)CHECK_SIZE+6, (float)CHECK_SIZE+6, 4, rgba(128,128,128, isActive(id)?196:96));
  if (checked)
  {
    if (enabled)
      addgfxcmdroundedrect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, rgba(255,255,255,isActive(id)?255:200));
    else
      addgfxcmdroundedrect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, rgba(128,128,128,200));
  }

  if (enabled)
    addgfxcmdtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxcmdtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  return res;
}

bool collapse(const char *text, const char *subtext, bool checked, bool enabled) {
  g_state.widgetId++;
  u32 id = (g_state.areaId<<16) | g_state.widgetId;

  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  int w = g_state.widgetW;
  int h = BUTTON_HEIGHT;
  g_state.widgetY -= BUTTON_HEIGHT; // + DEFAULT_SPACING;

  const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
  const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;

  bool over = enabled && inrect(x, y, w, h);
  bool res = buttonLogic(id, over);

  if (checked)
    addgfxcmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 2, rgba(255,255,255,isActive(id)?255:200));
  else
    addgfxcmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 1, rgba(255,255,255,isActive(id)?255:200));

  if (enabled)
    addgfxcmdtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  else
    addgfxcmdtext(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));

  if (subtext)
    addgfxcmdtext(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, subtext, rgba(255,255,255,128));

  return res;
}

void label(const char *text) {
  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  g_state.widgetY -= BUTTON_HEIGHT;
  addgfxcmdtext(x, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(255,255,255,255));
}

void value(const char *text) {
  const int x = g_state.widgetX;
  const int y = g_state.widgetY - BUTTON_HEIGHT;
  const int w = g_state.widgetW;
  g_state.widgetY -= BUTTON_HEIGHT;

  addgfxcmdtext(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, text, rgba(255,255,255,200));
}

bool slider(const char *text, float *val, float vmin, float vmax, float vinc, bool enabled)
{
  g_state.widgetId++;
  u32 id = (g_state.areaId<<16) | g_state.widgetId;

  int x = g_state.widgetX;
  int y = g_state.widgetY - BUTTON_HEIGHT;
  int w = g_state.widgetW;
  int h = SLIDER_HEIGHT;
  g_state.widgetY -= SLIDER_HEIGHT + DEFAULT_SPACING;

  addgfxcmdroundedrect((float)x, (float)y, (float)w, (float)h, 4.0f, rgba(0,0,0,128));

  const int range = w - SLIDER_MARKER_WIDTH;

  float u = (*val - vmin) / (vmax-vmin);
  if (u < 0) u = 0;
  if (u > 1) u = 1;
  int m = (int)(u  *range);

  bool over = enabled && inrect(x+m, y, SLIDER_MARKER_WIDTH, SLIDER_HEIGHT);
  bool res = buttonLogic(id, over);
  bool valChanged = false;

  if (isActive(id)) {
    if (g_state.wentActive) {
      g_state.dragX = g_state.mx;
      g_state.dragOrig = u;
    }
    if (g_state.dragX != g_state.mx) {
      u = g_state.dragOrig + (float)(g_state.mx - g_state.dragX) / (float)range;
      if (u < 0) u = 0;
      if (u > 1) u = 1;
      *val = vmin + u*(vmax-vmin);
      *val = floorf(*val/vinc+0.5f)*vinc; // Snap to vinc
      m = (int)(u  *range);
      valChanged = true;
    }
  }

  if (isActive(id))
    addgfxcmdroundedrect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, rgba(255,255,255,255));
  else
    addgfxcmdroundedrect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, isHot(id) ? rgba(255,196,0,128) : rgba(255,255,255,64));

  // TODO: fix this, take a look at 'nicenum'.
  int digits = (int)(ceilf(log10f(vinc)));
  char fmt[16];
  snprintf(fmt, 16, "%%.%df", digits >= 0 ? 0 : -digits);
  char msg[128];
  snprintf(msg, 128, fmt, *val);

  if (enabled) {
    addgfxcmdtext(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
    addgfxcmdtext(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, msg, isHot(id) ? rgba(255,196,0,255) : rgba(255,255,255,200));
  } else {
    addgfxcmdtext(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_LEFT, text, rgba(128,128,128,200));
    addgfxcmdtext(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, ALIGN_RIGHT, msg, rgba(128,128,128,200));
  }

  return res || valChanged;
}


void indent() {
  g_state.widgetX += INDENT_SIZE;
  g_state.widgetW -= INDENT_SIZE;
}

void unindent() {
  g_state.widgetX -= INDENT_SIZE;
  g_state.widgetW += INDENT_SIZE;
}

void separator() {
  g_state.widgetY -= DEFAULT_SPACING*3;
}

void separatorline() {
  int x = g_state.widgetX;
  int y = g_state.widgetY - DEFAULT_SPACING*2;
  int w = g_state.widgetW;
  int h = 1;
  g_state.widgetY -= DEFAULT_SPACING*4;

  addgfxcmdrect((float)x, (float)y, (float)w, (float)h, rgba(255,255,255,32));
}
void drawtext(int x, int y, int align, const char *text, u32 color) {
  addgfxcmdtext(x, y, align, text, color);
}
void drawline(float x0, float y0, float x1, float y1, float r, u32 color) {
  addgfxcmdline(x0, y0, x1, y1, r, color);
}
void drawrect(float x, float y, float w, float h, u32 color) {
  addgfxcmdrect(x, y, w, h, color);
}
void drawroundedrect(float x, float y, float w, float h, float r, u32 color) {
  addgfxcmdroundedrect(x, y, w, h, r, color);
}

/*-------------------------------------------------------------------------
 - rendering part
 -------------------------------------------------------------------------*/
// Some math headers don't have PI defined.
static const float PI = 3.14159265f;

void imguifree(void* ptr, void* userptr);
void* imguimalloc(size_t size, void* userptr);

#define STBTT_malloc(x,y)    imguimalloc(x,y)
#define STBTT_free(x,y)      imguifree(x,y)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

void imguifree(void* ptr, void* /*userptr*/)
{
  free(ptr);
}

void* imguimalloc(size_t size, void* /*userptr*/)
{
  return malloc(size);
}

static const unsigned TEMP_COORD_COUNT = 100;
static float g_tempCoords[TEMP_COORD_COUNT*2];
static float g_tempNormals[TEMP_COORD_COUNT*2];

static const int CIRCLE_VERTS = 8*4;
static float g_circleVerts[CIRCLE_VERTS*2];

static stbtt_bakedchar g_cdata[96]; // ASCII 32..126 is 95 glyphs
static GLuint g_ftex = 0;

inline unsigned int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  return (r) | (g << 8) | (b << 16) | (a << 24);
}

#define NEW_PATH 1
static void drawPolygon(const float* coords, unsigned numCoords, float r, unsigned int col)
{
  if (numCoords > TEMP_COORD_COUNT) numCoords = TEMP_COORD_COUNT;

  for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
  {
    const float* v0 = &coords[j*2];
    const float* v1 = &coords[i*2];
    float dx = v1[0] - v0[0];
    float dy = v1[1] - v0[1];
    float d = sqrtf(dx*dx+dy*dy);
    if (d > 0)
    {
      d = 1.0f/d;
      dx *= d;
      dy *= d;
    }
    g_tempNormals[j*2+0] = dy;
    g_tempNormals[j*2+1] = -dx;
  }

  for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
  {
    float dlx0 = g_tempNormals[j*2+0];
    float dly0 = g_tempNormals[j*2+1];
    float dlx1 = g_tempNormals[i*2+0];
    float dly1 = g_tempNormals[i*2+1];
    float dmx = (dlx0 + dlx1) * 0.5f;
    float dmy = (dly0 + dly1) * 0.5f;
    float  dmr2 = dmx*dmx + dmy*dmy;
    if (dmr2 > 0.000001f)
    {
      float  scale = 1.0f / dmr2;
      if (scale > 10.0f) scale = 10.0f;
      dmx *= scale;
      dmy *= scale;
    }
    g_tempCoords[i*2+0] = coords[i*2+0]+dmx*r;
    g_tempCoords[i*2+1] = coords[i*2+1]+dmy*r;
  }

  // unsigned int colTrans = RGBA(col&0xff, (col>>8)&0xff, (col>>16)&0xff, 0);
  const auto colTrans = vec4f(float(col&0xff), float((col>>8)&0xff), float((col>>16)&0xff), 0.f) / 255.f;
  const auto colf = vec4f(float(col&0xff), float((col>>8)&0xff), float((col>>16)&0xff), float(col>>24)) / 255.f;

#if NEW_PATH
  typedef array<float,6> verttype;
  vector<verttype> verts;
  for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++) {
    verts.add(verttype(colf,coords[i*2+0],coords[i*2+1]));
    verts.add(verttype(colf,coords[j*2+0],coords[j*2+1]));
    verts.add(verttype(colTrans,g_tempCoords[j*2+0],g_tempCoords[j*2+1]));
    verts.add(verttype(colTrans,g_tempCoords[j*2+0],g_tempCoords[j*2+1]));
    verts.add(verttype(colTrans,g_tempCoords[i*2+0],g_tempCoords[i*2+1]));
    verts.add(verttype(colf,coords[i*2+0],coords[i*2+1]));
  }

  for (unsigned i = 2; i < numCoords; ++i) {
    verts.add(verttype(colf, coords[0], coords[1]));
    verts.add(verttype(colf, coords[(i-1)*2], coords[(i-1)*2+1]));
    verts.add(verttype(colf, coords[i*2], coords[i*2+1]));
  }
  ogl::bindfixedshader(ogl::COLOR);
  ogl::immdraw(GL_TRIANGLES, 2, 0, 4, verts.length(), &verts[0][0]);
#else
  glBegin(GL_TRIANGLES);

  glColor4fv(&colf.x);

  for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
  {
    glVertex2fv(&coords[i*2]);
    glVertex2fv(&coords[j*2]);
    glColor4fv(&colTrans.x);
    glVertex2fv(&g_tempCoords[j*2]);

    glVertex2fv(&g_tempCoords[j*2]);
    glVertex2fv(&g_tempCoords[i*2]);

    glColor4fv(&colf.x);
    glVertex2fv(&coords[i*2]);
  }

  glColor4fv(&colf.x);
  for (unsigned i = 2; i < numCoords; ++i)
  {
    glVertex2fv(&coords[0]);
    glVertex2fv(&coords[(i-1)*2]);
    glVertex2fv(&coords[i*2]);
  }

  glEnd();
#endif
}

static void drawRect(float x, float y, float w, float h, float fth, unsigned int col)
{
  float verts[4*2] =
  {
    x+0.5f, y+0.5f,
    x+w-0.5f, y+0.5f,
    x+w-0.5f, y+h-0.5f,
    x+0.5f, y+h-0.5f,
  };
  drawPolygon(verts, 4, fth, col);
}

static void drawRoundedRect(float x, float y, float w, float h, float r, float fth, unsigned int col)
{
  const unsigned n = CIRCLE_VERTS/4;
  float verts[(n+1)*4*2];
  const float* cverts = g_circleVerts;
  float* v = verts;

  for (unsigned i = 0; i <= n; ++i)
  {
    *v++ = x+w-r + cverts[i*2]*r;
    *v++ = y+h-r + cverts[i*2+1]*r;
  }

  for (unsigned i = n; i <= n*2; ++i)
  {
    *v++ = x+r + cverts[i*2]*r;
    *v++ = y+h-r + cverts[i*2+1]*r;
  }

  for (unsigned i = n*2; i <= n*3; ++i)
  {
    *v++ = x+r + cverts[i*2]*r;
    *v++ = y+r + cverts[i*2+1]*r;
  }

  for (unsigned i = n*3; i < n*4; ++i)
  {
    *v++ = x+w-r + cverts[i*2]*r;
    *v++ = y+r + cverts[i*2+1]*r;
  }
  *v++ = x+w-r + cverts[0]*r;
  *v++ = y+r + cverts[1]*r;

  drawPolygon(verts, (n+1)*4, fth, col);
}

static void drawLine(float x0, float y0, float x1, float y1, float r, float fth, unsigned int col)
{
  float dx = x1-x0;
  float dy = y1-y0;
  float d = sqrtf(dx*dx+dy*dy);
  if (d > 0.0001f)
  {
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

  drawPolygon(verts, 4, fth, col);
}

bool renderglinit(const char* fontpath) {
  for (int i = 0; i < CIRCLE_VERTS; ++i) {
    float a = (float)i/(float)CIRCLE_VERTS * PI*2;
    g_circleVerts[i*2+0] = cosf(a);
    g_circleVerts[i*2+1] = sinf(a);
  }

  // Load font.
  FILE* fp = fopen(fontpath, "rb");
  if (!fp) return false;
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  unsigned char* ttfBuffer = (unsigned char*)malloc(size);
  if (!ttfBuffer) {
    fclose(fp);
    return false;
  }

  fread(ttfBuffer, 1, size, fp);
  fclose(fp);
  fp = 0;

  unsigned char* bmap = (unsigned char*)malloc(512*512);
  if (!bmap)
  {
    free(ttfBuffer);
    return false;
  }

  stbtt_BakeFontBitmap(ttfBuffer,0, 15.0f, bmap,512,512, 32,96, g_cdata);
#if NEW_PATH
  g_ftex = ogl::maketex("TB Ir Dr B2 Ml ml",bmap,512,512);
#else
  g_ftex = ogl::maketex("TB Ia Da B2 Ml ml",bmap,512,512);
#endif

  free(ttfBuffer);
  free(bmap);

  return true;
}

void rendergldestroy() {
  if (g_ftex) {
    glDeleteTextures(1, &g_ftex);
    g_ftex = 0;
  }
}

static void getBakedQuad(stbtt_bakedchar *chardata, int pw, int ph, int char_index,
                         float *xpos, float *ypos, stbtt_aligned_quad *q)
{
  stbtt_bakedchar *b = chardata + char_index;
  int round_x = STBTT_ifloor(*xpos + b->xoff);
  int round_y = STBTT_ifloor(*ypos - b->yoff);

  q->x0 = (float)round_x;
  q->y0 = (float)round_y;
  q->x1 = (float)round_x + b->x1 - b->x0;
  q->y1 = (float)round_y - b->y1 + b->y0;

  q->s0 = b->x0 / (float)pw;
  q->t0 = b->y0 / (float)pw;
  q->s1 = b->x1 / (float)ph;
  q->t1 = b->y1 / (float)ph;

  *xpos += b->xadvance;
}

static const float g_tabStops[4] = {150, 210, 270, 330};

static float getTextLength(stbtt_bakedchar *chardata, const char* text)
{
  float xpos = 0;
  float len = 0;
  while (*text)
  {
    int c = (unsigned char)*text;
    if (c == '\t') {
      for (int i = 0; i < 4; ++i)
        if (xpos < g_tabStops[i]) {
          xpos = g_tabStops[i];
          break;
        }
    } else if (c >= 32 && c < 128) {
      stbtt_bakedchar *b = chardata + c-32;
      int round_x = STBTT_ifloor((xpos + b->xoff) + 0.5);
      len = round_x + b->x1 - b->x0 + 0.5f;
      xpos += b->xadvance;
    }
    ++text;
  }
  return len;
}

static void drawText(float x, float y, const char *text, int align, u32 c)
{
  if (!g_ftex) return;
  if (!text) return;

  if (align == ALIGN_CENTER)
    x -= getTextLength(g_cdata, text)/2;
  else if (align == ALIGN_RIGHT)
    x -= getTextLength(g_cdata, text);

#if !NEW_PATH
  glColor4f(float(c&0xff)/255.f,
            float((c>>8)&0xff)/255.f,
            float((c>>16)&0xff)/255.f,
            float((c>>24)&0xff)/255.f);

  glEnable(GL_TEXTURE_2D);

  // assume orthographic projection with units = screen pixels, origin at top left
  glBindTexture(GL_TEXTURE_2D, g_ftex);

  glBegin(GL_TRIANGLES);

  const float ox = x;

  while (*text) {
    int c = (unsigned char)*text;
    if (c == '\t') {
      for (int i = 0; i < 4; ++i)
        if (x < g_tabStops[i]+ox) {
          x = g_tabStops[i]+ox;
          break;
        }
    } else if (c >= 32 && c < 128) {
      stbtt_aligned_quad q;
      getBakedQuad(g_cdata, 512,512, c-32, &x,&y,&q);

      glTexCoord2f(q.s0, q.t0);
      glVertex2f(q.x0, q.y0);
      glTexCoord2f(q.s1, q.t1);
      glVertex2f(q.x1, q.y1);
      glTexCoord2f(q.s1, q.t0);
      glVertex2f(q.x1, q.y0);

      glTexCoord2f(q.s0, q.t0);
      glVertex2f(q.x0, q.y0);
      glTexCoord2f(q.s0, q.t1);
      glVertex2f(q.x0, q.y1);
      glTexCoord2f(q.s1, q.t1);
      glVertex2f(q.x1, q.y1);
    }
    ++text;
  }

  glEnd();
  glDisable(GL_TEXTURE_2D);
#else
  const vec4f cf(float(c&0xff)/255.f,
                 float((c>>8)&0xff)/255.f,
                 float((c>>16)&0xff)/255.f,
                 float((c>>24)&0xff)/255.f);

  const float ox = x;
  vector<vec4f> verts;
  while (*text) {
    int c = (unsigned char)*text;
    if (c == '\t') {
      for (int i = 0; i < 4; ++i)
        if (x < g_tabStops[i]+ox) {
          x = g_tabStops[i]+ox;
          break;
        }
    } else if (c >= 32 && c < 128) {
      stbtt_aligned_quad q;
      getBakedQuad(g_cdata, 512,512, c-32, &x,&y,&q);

      verts.add(vec4f(q.s0, q.t0, q.x0, q.y0));
      verts.add(vec4f(q.s1, q.t1, q.x1, q.y1));
      verts.add(vec4f(q.s1, q.t0, q.x1, q.y0));
      verts.add(vec4f(q.s0, q.t0, q.x0, q.y0));
      verts.add(vec4f(q.s0, q.t1, q.x0, q.y1));
      verts.add(vec4f(q.s1, q.t1, q.x1, q.y1));
    }
    ++text;
  }

  ogl::bindfixedshader(ogl::DIFFUSETEX|ogl::COLOR);
  OGL(Enable, GL_TEXTURE_2D);
  ogl::bindtexture(GL_TEXTURE_2D, g_ftex);
  OGL(VertexAttrib4fv, ogl::COL, &cf.x);
  ogl::immdraw(GL_TRIANGLES, 2, 2, 0, verts.length(), &verts[0].x);
  OGL(Disable, GL_TEXTURE_2D);
#endif
}

void rendergldraw(int width, int height)
{
  const gfxcmd* q = getrenderqueue();
  int nq = getrenderqueuesize();

  const float s = 1.0f/8.0f;

  glDisable(GL_SCISSOR_TEST);
  for (int i = 0; i < nq; ++i) {
    const gfxcmd& cmd = q[i];
    if (cmd.type == GFXCMD_RECT) {
      if (cmd.rect.r == 0)
        drawRect((float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
             (float)cmd.rect.w*s-1, (float)cmd.rect.h*s-1,
             1.0f, cmd.col);
      else
        drawRoundedRect((float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
                (float)cmd.rect.w*s-1, (float)cmd.rect.h*s-1,
                (float)cmd.rect.r*s, 1.0f, cmd.col);
    }
    else if (cmd.type == GFXCMD_LINE)
      drawLine(cmd.line.x0*s, cmd.line.y0*s, cmd.line.x1*s, cmd.line.y1*s, cmd.line.r*s, 1.0f, cmd.col);
    else if (cmd.type == GFXCMD_TRIANGLE) {
      if (cmd.flags == 1) {
        const float verts[3*2] =
        {
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s-1, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s/2-0.5f,
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
        };
        drawPolygon(verts, 3, 1.0f, cmd.col);
      }
      if (cmd.flags == 2) {
        const float verts[3*2] = {
          (float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s/2-0.5f, (float)cmd.rect.y*s+0.5f,
          (float)cmd.rect.x*s+0.5f+(float)cmd.rect.w*s-1, (float)cmd.rect.y*s+0.5f+(float)cmd.rect.h*s-1,
        };
        drawPolygon(verts, 3, 1.0f, cmd.col);
      }
    } else if (cmd.type == GFXCMD_TEXT)
      drawText(cmd.text.x, cmd.text.y, cmd.text.text, cmd.text.align, cmd.col);
    else if (cmd.type == GFXCMD_SCISSOR) {
      if (cmd.flags) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h);
      } else
        glDisable(GL_SCISSOR_TEST);
    }
  }
  glDisable(GL_SCISSOR_TEST);
}
} /* namespace ui */
} /* namespace q */
#endif

