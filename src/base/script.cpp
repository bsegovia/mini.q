/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - script.cpp -> implements embedded script / con language
 -------------------------------------------------------------------------*/
#include "sys.hpp"
#include "script.hpp"
#include "client.hpp"
#include "console.hpp"
#include "hash_map.hpp"
#include "base/lua/lualib.h"
#include <cstdio>

namespace q {
namespace script {

static void *internalalloc(void*, void *ptr, size_t, size_t nsize) {
  return sys::memrealloc(ptr, nsize, __FILE__, __LINE__);
}

lua_State *luastate() {
  static lua_State *L = NULL;
  if (L == NULL) {
    L = lua_newstate(internalalloc, NULL);
    luaL_openlibs(L);
  }
  return L;
}

struct identifier {
  int *storage;
  bool persist;
};

// tab-completion of all idents
static int completesize = 0, completeidx = 0;

// contains all vars/commands/aliases
typedef hash_map<string, identifier> identifier_map;
static identifier_map *idents = NULL;

void finish(void) {
  lua_close(luastate());
  SAFE_DEL(idents);
  idents = NULL;
}

static void initializeidents(void) {
  if (!idents) idents = NEWE(identifier_map);
}

int variable(const char *name, int min, int cur, int max,
             int *storage, void (*fun)(), bool persist) {
  initializeidents();
  const identifier v = {storage, persist};
  idents->insert(makepair(name, v));
  luabridge::getGlobalNamespace(luastate())
    .beginNamespace("q")
      .addVariable(name, storage, min, max, fun)
    .endNamespace();
  return cur;
}

bool addcommand(const char *name) {
  initializeidents();
  const identifier c = {NULL, false};
  idents->insert(makepair(name, c));
  return false;
}

static int luareport(int ret) {
  auto L = luastate();
  if (ret && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg != NULL) con::out("lua script failed with %s", msg);
    lua_pop(L, 1);
  }
  return ret;
}

int execstring(const char *p) {
  auto L = luastate();
  if (luareport(luaL_loadstring(L, p))) return 0;
  return luareport(lua_pcall(L, 0, 0, 0));
}

bool execfile(const char *cfgfile) {
  fixedstring s(cfgfile);
  const auto buf = sys::loadfile(sys::path(s.c_str()), NULL);
  if (!buf) {
    con::out("unable to find %s", cfgfile);
    return false;
  }
  execstring(buf);
  FREE(buf);
  return true;
}

void execscript(const char *cfgfile) {
  if (!execfile(cfgfile))
    con::out("could not read \"%s\"", cfgfile);
}
CMD(execscript);

void resetcomplete(void) { completesize = 0; }
void complete(fixedstring &s) {
  if (*s.c_str()!='/') {
    fixedstring t;
    strcpy_s(t, s.c_str());
    strcpy_s(s, "/");
    strcat_s(s, t.c_str());
  }
  if (!s[1]) return;
  if (!completesize) {
    completesize = int(strlen(s.c_str())-1);
    completeidx = 0;
  }
  int idx = 0;
  for (auto it = idents->begin(); it != idents->end(); ++it)
    if (strncmp(it->first.c_str(), s.c_str()+1, completesize)==0 && idx++==completeidx)
      s.fmt("/%s", it->first.c_str());
  completeidx++;
  if (completeidx>=idx) completeidx = 0;
}

void writecfg(void) {
  auto f = fopen("config.q", "w");
  if (!f) return;
  fprintf(f, "-- automatically written on exit, do not modify\n"
             "-- delete this file to have defaults.lua overwrite these settings\n"
             "-- modify settings in game, or put settings in autoexec.lua to override anything\n\n");
  for (auto it = idents->begin(); it != idents->end(); ++it)
    if (it->second.persist)
      fprintf(f, "q.%s = %d\n", it->first.c_str(), *it->second.storage);
  fclose(f);
}
CMD(writecfg);
} /* namespace cmd */
} /* namespace q */

