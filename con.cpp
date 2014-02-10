/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - con.cpp -> implements console functionalities
 -------------------------------------------------------------------------*/
#include "con.hpp"
#include "game.hpp"
#include "script.hpp"
#include "text.hpp"
#include "sys.hpp"

namespace q {
namespace con {
struct cline { char *cref; float outtime; };
static vector<cline> conlines;
static const int ndraw = 5;
static const unsigned int WORDWRAP = 80;
static int conskip = 0;
static bool saycommandon = false;
static string cmdbuf;
static cvector vhistory;
static int histpos = 0;

bool active() { return saycommandon; }
static void setconskip(const int &n) {
  conskip += n;
  if (conskip < 0) conskip = 0;
}
CMDN(conskip, setconskip, "i");

// keymap is defined externally in keymap.q
struct keym { int code; char *name; char *action; } keyms[256];
static int numkm = 0;

static void keymap(const char *code, const char *key, const char *action) {
  keyms[numkm].code = atoi(code);
  keyms[numkm].name = NEWSTRING(key);
  keyms[numkm++].action = NEWSTRINGBUF(action);
}
CMD(keymap, "sss");

static void bindkey(char *key, char *action) {
  for (char *x = key; *x; x++) *x = toupper(*x);
  loopi(numkm) if (strcmp(keyms[i].name, key)==0) {
    strcpy_cs(keyms[i].action, action);
    return;
  }
  con::out("unknown key \"%s\"", key);
}
CMDN(bind, bindkey, "ss");

void finish() {
  loopv(conlines) FREE(conlines[i].cref);
  loopv(vhistory) FREE(vhistory[i]);
  vhistory.destroy();
  conlines.destroy();
  loopi(numkm) {
    FREE(keyms[i].name);
    FREE(keyms[i].action);
  }
}

static void line(const char *sf, bool highlight) {
  cline cl;
  cl.cref = conlines.length()>100 ? conlines.pop().cref : NEWSTRINGBUF("");
  cl.outtime = game::lastmillis; // for how long to keep line on screen
  conlines.insert(0,cl);
  if (highlight) { // show line in a different colour, for chat etc.
    cl.cref[0] = '\f';
    cl.cref[1] = 0;
    strcat_cs(cl.cref, sf);
  } else
    strcpy_cs(cl.cref, sf);
  puts(cl.cref);
#if defined(__WIN32__)
  fflush(stdout);
#endif
}

void out(const char *s, ...) {
  sprintf_sdv(sf, s);
  s = sf;
  int n = 0;
  while (strlen(s)>WORDWRAP) { // cut strings to fit on screen
    string t;
    strn0cpy(t, s, WORDWRAP+1);
    line(t, n++!=0);
    s += WORDWRAP;
  }
  line(s, n!=0);
}

FVAR(confadeout, 1.f, 5000.f, 256000.f);
void render() {
  int nd = 0;
  char *refs[ndraw];
  loopv(conlines) {
    if (conskip ? i>=conskip-1 || i>=conlines.length()-ndraw :
       game::lastmillis-conlines[i].outtime < confadeout) {
      refs[nd++] = conlines[i].cref;
      if (nd==ndraw) break;
    }
  }
  const auto font = text::fontdim();
  const auto old = text::displaydim().x;
  text::displaywidth(font.x);
  loopj(nd) {
    const vec2f pos(font.x, font.y*float(nd-j-1));
    text::draw(refs[j], pos);
  }
  text::displaywidth(old);
}

// turns input to the command line on or off
static void saycommand(const char *init) {
  SDL_EnableUNICODE(saycommandon = (init!=NULL));
  sys::keyrepeat(saycommandon);
  if (!init) init = "";
  strcpy_s(cmdbuf, init);
}
CMD(saycommand, "s");

static void history(const int &n) {
  static bool rec = false;
  if (!rec && n>=0 && n<vhistory.length()) {
    rec = true;
    script::execstring(vhistory[vhistory.length()-n-1]);
    rec = false;
  }
}
CMD(history, "i");

void keypress(int code, int isdown, int cooked) {
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
        case SDLK_UP:
          if (histpos) strcpy_s(cmdbuf, vhistory[--histpos]); break;
        case SDLK_DOWN:
          if (histpos<vhistory.length()) strcpy_s(cmdbuf, vhistory[histpos++]); break;
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
          if (vhistory.empty() || strcmp(vhistory.last(), cmdbuf))
            vhistory.add(NEWSTRING(cmdbuf));  // cap this?
          histpos = vhistory.length();
          if (cmdbuf[0]=='/')
            script::execstring(cmdbuf, isdown);
          /* else client::toserver(cmdbuf); */
        }
        saycommand(NULL);
      } else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else /* if (!menu::key(code, isdown) */ { // keystrokes go to menu
    loopi(numkm) if (keyms[i].code==code) { // keystrokes go to game, lookup in keymap and execute
      script::execstring(keyms[i].action, isdown);
      return;
    }
  }
}
const char *curcmd() { return saycommandon ? cmdbuf : NULL; }
} /* namespace con */
} /* namespace q */

