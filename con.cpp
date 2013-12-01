/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - con.cpp -> implements console functionalities
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "game.hpp"
#include "script.hpp"
#include "sys.hpp"

namespace q {
namespace con {
static ringbuffer<pair<string,float>, 128> buffer;

static void line(const char *sf, bool highlight) {
  auto &cl = buffer.prepend();
  cl.second = game::lastmillis;
  if (highlight) sprintf_s(cl.first)("\f%s", sf);
  else strcpy_s(cl.first, sf);
  puts(cl.first);
}

void out(const char *s, ...) {
  sprintf_sdv(sf, s);
  s = sf;
  int n = 0;
  while (strlen(s) > WORDWRAP) { // cut strings to fit on screen
    string t;
    strn0cpy(t, s, WORDWRAP+1);
    line(t, n++!=0);
    s += WORDWRAP;
  }
  line(s, n!=0);
}

// keymap is defined externally in keymap.q
struct keym { int code; const char *name; string action; } keyms[256];
static int numkm = 0;
static void keymap(const char *code, const char *key, const char *action) {
  static linear_allocator<4096,sizeof(char)> lin;
  keyms[numkm].code = atoi(code);
  keyms[numkm].name = lin.newstring(key);
  sprintf_s(keyms[numkm++].action)("%s", action);
}
CMD(keymap, "sss");

static void bindkey(char *key, char *action) {
  for (char *x = key; *x; x++) *x = toupper(*x);
  loopi(numkm) if (strcmp(keyms[i].name, key)==0) {
    strcpy_s(keyms[i].action, action);
    return;
  }
  con::out("unknown key \"%s\"", key);
}
CMDN(bind, bindkey, "ss");

static const int ndraw = 5;
static int conskip = 0;
static bool saycommandon = false;
static string cmdbuf;

const char *curcmd() { return saycommandon?cmdbuf:NULL; }
static void setconskip(const int &n) {
  conskip += n;
  if (conskip < 0) conskip = 0;
}
CMDN(conskip, setconskip, "i");

static void saycommand(const char *init) {
  SDL_EnableUNICODE(saycommandon = (init!=NULL));
  /* if (!editmode) */ sys::keyrepeat(saycommandon);
  if (!init) init = "";
  strcpy_s(cmdbuf, init);
}
CMD(saycommand, "s");

void keypress(int code, bool isdown, int cooked) {
  static ringbuffer<string, 64> h;
  static int hpos = 0;
  if (saycommandon) { // keystrokes go to commandline
    if (isdown) {
      switch (code) {
        case SDLK_RETURN: break;
        case SDLK_BACKSPACE:
        case SDLK_LEFT: {
          loopi(cmdbuf[i]) if (!cmdbuf[i+1]) cmdbuf[i] = 0;
          script::resetcomplete();
          break;
        }
        case SDLK_UP: if (hpos) strcpy_s(cmdbuf, h[--hpos]); break;
        case SDLK_DOWN: if (hpos<int(h.n)) strcpy_s(cmdbuf, h[hpos++]); break;
        case SDLK_TAB: script::complete(cmdbuf); break;
        default:
          script::resetcomplete();
          if (cooked) {
            const char add[] = { char(cooked), 0 };
            strcat_s(cmdbuf, add);
          }
      }
    } else {
      if (code==SDLK_RETURN) {
        if (cmdbuf[0]) {
          if (h.empty() || strcmp(h.back(), cmdbuf))
            strcpy_s(h.append(), cmdbuf);
          hpos = h.n;
          if (cmdbuf[0]=='/')
            script::execstring(cmdbuf, true);
          else
            /*client::toserver(cmdbuf)*/;
        }
        saycommand(NULL);
      }
      else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else /* if (!menu::key(code, isdown))*/ { // keystrokes go to menu
    loopi(numkm) if (keyms[i].code==code) { // keystrokes go to game
      script::execstring(keyms[i].action);
      return;
    }
  }
}
} /* namespace con */
} /* namespace q */

