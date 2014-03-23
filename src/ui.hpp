/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ui.hpp -> exposes immediate mode rendering
 - taken from http://github.com/AdrienHerubel/imgui 
 -------------------------------------------------------------------------*/
#include "base/sys.hpp"

namespace q {
namespace ui {

  enum MouseButton {
    MBUT_LEFT = 0x01, 
    MBUT_RIGHT = 0x02, 
  };

  enum TextAlign {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
  };

  INLINE u32 rgba(u8 r, u8 g, u8 b, u8 a=255) {return r|(g<<8)|(b<<16)|(a<<24);}

  void beginframe(int mx, int my, u8 mbut, int scroll);
  void endframe();

  bool beginscrollarea(const char *name, int x, int y, int w, int h, int *scroll);
  void endscrollarea();

  void indent();
  void unindent();
  void separator();
  void separatorline();

  bool button(const char *text, bool enabled = true);
  bool item(const char *text, bool enabled = true);
  bool check(const char *text, bool checked, bool enabled = true);
  bool collapse(const char *text, const char *subtext, bool checked, bool enabled = true);
  void label(const char *text);
  void value(const char *text);
  bool slider(const char *text, float *val, float vmin, float vmax, float vinc, bool enabled = true);

  void drawtext(int x, int y, int align, const char *text, u32 color);
  void drawline(float x0, float y0, float x1, float y1, float r, u32 color);
  void drawroundedrect(float x, float y, float w, float h, float r, u32 color);
  void drawrect(float x, float y, float w, float h, u32 color);
  void draw(int w, int h);
} /* namespace ui */
} /* namespace q */

