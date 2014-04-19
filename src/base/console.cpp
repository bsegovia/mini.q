/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - con.cpp -> implements console functionalities
 -------------------------------------------------------------------------*/
#include "console.hpp"
#include "game.hpp"
#include "script.hpp"
#include "client.hpp"
#include "menu.hpp"
#include "text.hpp"
#include "sys.hpp"

namespace q {
namespace con {
struct cline { char *cref; int outtime; };
static vector<cline> conlines;
static const int ndraw = 5;
static const unsigned int WORDWRAP = 80;
static int conskip = 0;
static bool saycommandon = false;
static fixedstring cmdbuf;
static cvector vhistory;
static int histpos = 0;

bool active() { return saycommandon; }
static void setconskip(int n) {
  conskip += n;
  if (conskip < 0) conskip = 0;
}
CMDN(conskip, setconskip, ARG_1INT);

// keymap is defined externally in keymap.q
struct keym { int code; char *name; char *action; } keyms[256];
static int numkm = 0;

static void keymap(const char *code, const char *key, const char *action) {
  keyms[numkm].code = ATOI(code);
  keyms[numkm].name = NEWSTRING(key);
  keyms[numkm++].action = NEWSTRINGBUF(action);
}
CMD(keymap, ARG_3STR);

static void bindkey(char *key, char *action) {
  for (char *x = key; *x; x++) *x = toupper(*x);
  loopi(numkm) if (strcmp(keyms[i].name, key)==0) {
    strcpy_cs(keyms[i].action, action);
    return;
  }
  con::out("unknown key \"%s\"", key);
}
CMDN(bind, bindkey, ARG_2STR);

#if !defined(RELEASE)
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
#endif

static void line(const char *sf, bool highlight) {
  cline cl;
  cl.cref = conlines.length()>100 ? conlines.pop().cref : NEWSTRINGBUF("");
  cl.outtime = int(game::lastmillis()); // for how long to keep line on screen
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
  vasprintfsd(sf, s, s);
  s = sf.c_str();
  int n = 0;
  while (strlen(s)>WORDWRAP) { // cut strings to fit on screen
    fixedstring t;
    strn0cpy(t.c_str(), s, WORDWRAP+1);
    line(t.c_str(), n++!=0);
    s += WORDWRAP;
  }
  line(s, n!=0);
}

VAR(confadeout, 1, 5000, 256000);
static int conlinenum(char *refs[ndraw]) {
  int nd = 0;
  loopv(conlines) {
    if (conskip ? i>=conskip-1 || i>=conlines.length()-ndraw :
       game::lastmillis()-conlines[i].outtime < confadeout) {
      refs[nd] = conlines[i].cref;
      if (++nd==ndraw) break;
    }
  }
  return nd;
}

float height() {
  char *refs[ndraw];
  const auto cmd = curcmd();
  const int nd = conlinenum(refs) + (cmd?1:0);
  return float(sys::scrh)-nd*text::fontdim().y;
}

void render() {
  char *refs[ndraw];
  const int nd = conlinenum(refs);

  // console output
  const auto font = text::fontdim();
  text::displaywidth(font.x);
  loopj(nd) text::draw(refs[j], font.x, float(sys::scrh)-font.y*(j+1));

  // command line
  const auto cmd = curcmd();
  if (cmd) text::drawf("> %s_", vec2f(font.x, float(sys::scrh)-font.y*(nd+1)), cmd);
}

// turns input to the command line on or off
static void saycommand(const char *init) {
  saycommandon = (init!=NULL);
  sys::textinput(saycommandon);
  sys::keyrepeat(saycommandon);
  if (!init) init = "";
  strcpy_s(cmdbuf, init);
}
CMD(saycommand, ARG_1STR);

static void history(int n) {
  static bool rec = false;
  if (!rec && n>=0 && n<vhistory.length()) {
    rec = true;
    const auto cmd = (const char*)(vhistory[vhistory.length()-n-1]);
    script::execstring(cmd);
    rec = false;
  }
}
CMD(history, ARG_1INT);

void processtextinput(const char *str) {
  script::resetcomplete();
  strcat_s(cmdbuf, str);
}

void keypress(int code, bool isdown) {
  if (saycommandon) { // keystrokes go to commandline
    if (isdown) {
      switch (code) {
        case SDLK_RETURN: break;
        case SDLK_BACKSPACE:
        case SDLK_LEFT:
          loopi(cmdbuf[i]) if (!cmdbuf[i+1]) cmdbuf[i] = 0;
          script::resetcomplete();
        break;
        case SDLK_UP:
          if (histpos) strcpy_s(cmdbuf, vhistory[--histpos]);
        break;
        case SDLK_DOWN:
          if (histpos<vhistory.length()) strcpy_s(cmdbuf, vhistory[histpos++]);
        break;
        case SDLK_TAB: script::complete(cmdbuf); break;
        default:
          script::resetcomplete();
        break;
      }
    } else {
      if (code==SDLK_RETURN) {
        if (cmdbuf[0]) {
          if (vhistory.empty() || strcmp(vhistory.last(), cmdbuf.c_str()))
            vhistory.add(NEWSTRING(cmdbuf.c_str()));  // cap this?
          histpos = vhistory.length();
          if (cmdbuf[0]=='/')
            script::execstring(cmdbuf.c_str(), true);
          else
            client::toserver(cmdbuf.c_str());
        }
        saycommand(NULL);
      } else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else if (!menu::key(code, isdown)) { // keystrokes go to menu
    loopi(numkm) if (keyms[i].code==code) { // keystrokes go to game, lookup in keymap and execute
      script::execstring(keyms[i].action, isdown);
      return;
    }
  }
}
const char *curcmd() { return saycommandon ? cmdbuf.c_str() : NULL; }
} /* namespace con */
} /* namespace q */

