/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - menu.cpp -> implements simple menu handling
 -------------------------------------------------------------------------*/
#include "menu.hpp"
#include "game.hpp"
#include "script.hpp"
#include "client.hpp"
#include "serverbrowser.hpp"
#include "sys.hpp"
#include "stl.hpp"
#include <SDL/SDL.h>

namespace q {
namespace menu {

struct mitem {
  char *text, *action;
  s32 manual;
};
struct gmenu {
  char *name;
  vector<mitem> items;
  s32 menusel;
};

static vector<gmenu> menus;
static int vmenu = -1;
static ivector menustack;
static string empty = "";

void set(int menu) {
  if ((vmenu = menu)>=1)
    game::resetmovement(game::player1);
  if (vmenu==1)
    menus[1].menusel = 0;
}

static void show(const char *name) {
  loopv(menus) if (i>1 && strcmp(menus[i].name, name)==0) {
    set(i);
    return;
  }
}
CMDN(showmenu, show, ARG_1STR);

void clean(void) {
  loopv(menus) {
    FREE(menus[i].name);
    loopvj(menus[i].items) if (!menus[i].items[j].manual) {
      FREE(menus[i].items[j].text);
      FREE(menus[i].items[j].action);
    }
  }
}
static int compare(const mitem *a, const mitem *b) {
  const int x = atoi(a->text);
  const int y = atoi(b->text);
  if (x>y) return -1;
  if (x<y) return 1;
  return 0;
}

void sort(int start, int num) {
  qsort(&menus[0].items[start], num,
    sizeof(mitem),(int (CDECL *)(const void *,const void *))compare);
}

bool render(void) {
#if 0 // TODO
  if (vmenu<0) {
    menustack.setsize(0);
    return false;
  }
  if (vmenu==1)
    browser::refreshservers();
  gmenu &m = menus[vmenu];
  sprintf_sd(title)(vmenu>1 ? "[ %s menu ]" : "%s", m.name);
  int mdisp = m.items.length();
  int w = 0;
  loopi(mdisp) {
    const int x = rr::textwidth(m.items[i].text);
    if (x>w) w = x;
  }

  int tw = rr::textwidth(title);
  if (tw>w) w = tw;
  const int fh = rr::FONTH;
  int step = fh/4*5;
  int h = (mdisp+2)*step;
  int y = (rr::VIRTH-h)/2;
  int x = (rr::VIRTW-w)/2;
  rr::blendbox(x-fh/2*3, y-fh, x+w+fh/2*3, y+h+fh, true);
  rr::drawtext(title, x, y,2);
  y += fh*2;

  if (vmenu) {
    int bh = y+m.menusel*step;
    rr::blendbox(x-fh, bh-10, x+w+fh, bh+fh+10, false);
  }
  loopj(mdisp) {
    rr::drawtext(m.items[j].text, x, y, 2);
    y += step;
  }
#endif
  return true;
}

void newm(const char *name) {
  gmenu &menu = menus.add();
  menu.name = NEWSTRING(name);
  menu.menusel = 0;
}
CMDN(newmenu, newm, ARG_1STR);

void manual(int m, int n, char *text) {
  if (!n) menus[m].items.setsize(0);
  mitem &mi = menus[m].items.add();
  mi.text = text;
  mi.action = empty;
  mi.manual = 1;
}

void item(char *text, char *action) {
  gmenu &menu = menus.last();
  mitem &mi = menu.items.add();
  mi.text = NEWSTRING(text);
  mi.action = action[0] ? NEWSTRING(action) : NEWSTRING(text);
  mi.manual = 0;
}
CMDN(menuitem, item, ARG_2STR);

bool key(int code, bool isdown) {
  if (vmenu<=0) return false;
  int menusel = menus[vmenu].menusel;
  if (isdown) {
    if (code==SDLK_ESCAPE) {
      set(-1);
      if (!menustack.empty())
        set(menustack.pop());
      return true;
    }
    else if (code==SDLK_UP || code==-4) menusel--;
    else if (code==SDLK_DOWN || code==-5) menusel++;
    int n = menus[vmenu].items.length();
    if (menusel<0) menusel = n-1;
    else if (menusel>=n) menusel = 0;
    menus[vmenu].menusel = menusel;
  } else {
    if (code==SDLK_RETURN || code==-2) {
      char *action = menus[vmenu].items[menusel].action;
      if (vmenu==1)
        client::connect(browser::getservername(menusel));
      menustack.add(vmenu);
      set(-1);
      script::execstring(action, true);
    }
  }
  return true;
}
} /* namespace menu */
} /* namespace q */

