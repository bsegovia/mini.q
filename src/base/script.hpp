/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - script.hpp -> exposes embedded script / con language
 -------------------------------------------------------------------------*/
#pragma once
#include "base/string.hpp"
#include "base/lua/bridge/luabridge.hpp"

// mini-scripting language implemented in q (mostly cvompatible with quake
// script engine)
namespace q {
namespace script {

// return complete lua state
lua_State *luastate();

// function signatures for script functions
enum {
  ARG_1INT, ARG_2INT, ARG_3INT, ARG_4INT,
  ARG_NONE,
  ARG_1STR, ARG_2STR, ARG_3STR, ARG_5STR,
  ARG_DOWN, ARG_DWN1,
  ARG_1EXP, ARG_2EXP,
  ARG_1EST, ARG_2EST,
  ARG_VARI
};
// register a console variable (done through globals)
int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist);
// register a new command
bool addcommand(const char *name);
// stop completion
void resetcomplete();
// complete the given string
void complete(fixedstring &s);
// write all commands, variables and alias to config.cfg
void writecfg();
// free all resources needed by the command system
void finish();

// execute a given string
int executelua(const char *p);
// execute a given file and print any error
void execscript(const char *cfgfile);
// execute a file and says if this succeeded
bool execluascript(const char *cfgfile);

} /* namespace script */
} /* namespace q */

struct luainitializer {
  template <typename T> luainitializer(T) {}
};

// register a command with a given name
#define CMDN(name, fun, nargs) \
  static auto __dummy_##fun = q::script::addcommand(#name);\
  static auto __dummy_##fun##_lua =\
  luainitializer(q::luabridge::getGlobalNamespace(q::script::luastate())\
    .beginNamespace("q")\
    .addFunction(#name, fun)\
    .endNamespace())

// register a command with a name given by the function name
#define CMD(name, nargs) CMDN(name, name, nargs)

// a persistent variable
#define VARP(name, min, cur, max) \
  int name = q::script::variable(#name, min, cur, max, &name, NULL, true);

// a non-persistent variable
#define VAR(name, min, cur, max) \
  int name = q::script::variable(#name, min, cur, max, &name, NULL, false)

// a non-persistent variable with custom code to run when changed
#define VARF(name, min, cur, max, body) \
  void var_##name(); \
  int name = q::script::variable(#name, min, cur, max, &name, var_##name, false); \
  void var_##name() { body; }

// a persistent variable with custom code to run when changed
#define VARFP(name, min, cur, max, body) \
  void var_##name(); \
  int name = q::script::variable(#name, min, cur, max, &name, var_##name, true); \
  void var_##name() { body; }

