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
lua_State *getluastate();

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
// set the integer value for a variable
void setvar(const char *name, int i);
// set the value of a variable
int getvar(const char *name);
// says if the variable exists (i.e. is registered)
bool identexists(const char *name);
// register a new command
bool addcommand(const char *name, void (*fun)(), int narg);
// execute a given string
int execstring(const char *p, bool down = true);
// execute a given file and print any error
void exec(const char *cfgfile);
// execute a file and says if this succeeded
bool execfile(const char *cfgfile);
// stop completion
void resetcomplete();
// complete the given string
void complete(fixedstring &s);
// set an alias with given action
void alias(const char *name, const char *action);
// get the action string for the given variable
char *getalias(const char *name);
// write all commands, variables and alias to config.cfg
void writecfg();
// free all resources needed by the command system
void finish();
} /* namespace script */
} /* namespace q */

// register a command with a given name
#define CMDN(name, fun, nargs) \
  bool __dummy_##fun = q::script::addcommand(#name, (void (*)())fun, script::nargs)

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

