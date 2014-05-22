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
#include "hash_map.hpp"
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
static vector<char*> vhistory;
static int histpos = 0;

bool active() { return saycommandon; }
static void setconskip(int n) {
  conskip += n;
  if (conskip < 0) conskip = 0;
}
CMDN(conskip, setconskip);

// keymap is defined externally in keymap.q
static hash_map<string,int> key_map; // keyname -> code
static hash_map<int,string> action_map; // code -> action
static void insertkeymap(int code, const string &name) {
  key_map.insert(makepair(name,code));
}
CMDN(keymap, insertkeymap);

static void bindkey(const string &key, const string &action) {
  const auto it = key_map.find(key);
  if (it == key_map.end())
    out("unknown key \"%s\"", key.c_str());
  else
    action_map.insert(makepair(it->second, action));
}
CMDN(bind, bindkey);

#if !defined(RELEASE)
void finish() {
  loopv(conlines) FREE(conlines[i].cref);
  loopv(vhistory) FREE(vhistory[i]);
  vhistory = vector<char*>();
  conlines = vector<cline>();
}
#endif

static void line(const char *sf, bool highlight) {
  cline cl;
  if (conlines.size()>100) {
    cl.cref = conlines.back().cref;
    conlines.pop_back();
  } else
    cl.cref = NEWSTRINGBUF("");
  cl.outtime = int(game::lastmillis()); // for how long to keep line on screen
  conlines.insert(conlines.begin(),cl);
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
    if (conskip ? i>=conskip-1 || i>=conlines.size()-ndraw :
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
CMD(saycommand);

static void history(int n) {
  static bool rec = false;
  if (!rec && n>=0 && n<vhistory.size()) {
    rec = true;
    const auto cmd = (const char*)(vhistory[vhistory.size()-n-1]);
    script::execstring(cmd);
    rec = false;
  }
}
CMD(history);

void processtextinput(const char *str) {
  script::resetcomplete();
  strcat_s(cmdbuf, str);
}

static bool iskeydownflag = false;
void setkeydownflag(bool on) { iskeydownflag = on; }
bool iskeydown() { return iskeydownflag; }

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
          if (histpos<vhistory.size()) strcpy_s(cmdbuf, vhistory[histpos++]);
        break;
        case SDLK_TAB: script::complete(cmdbuf); break;
        default:
          script::resetcomplete();
        break;
      }
    } else {
      if (code==SDLK_RETURN) {
        if (cmdbuf[0]) {
          if (vhistory.empty() || strcmp(vhistory.back(), cmdbuf.c_str()))
            vhistory.push_back(NEWSTRING(cmdbuf.c_str()));  // cap this?
          histpos = vhistory.size();
          if (cmdbuf[0]=='/') {
            setkeydownflag(true);
            script::execstring(cmdbuf.c_str()+1);
          } else
            client::toserver(cmdbuf.c_str());
        }
        saycommand(NULL);
      } else if (code==SDLK_ESCAPE)
        saycommand(NULL);
    }
  } else if (!menu::key(code, isdown)) { // keystrokes go to menu
    const auto it = action_map.find(code);
    if (it != action_map.end()) script::execstring(it->second.c_str());
  }
}
const char *curcmd() { return saycommandon ? cmdbuf.c_str() : NULL; }
} /* namespace con */
} /* namespace q */

