/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - script.hpp -> exposes embedded script / console language
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "stl.hpp"

namespace q {
namespace script {
typedef void (CDECL *cb)();
int ivar(const char *n, int m, int cur, int M, int *ptr, cb fun, bool persist);
float fvar(const char *n, float m, float cur, float M, float *ptr, cb fun, bool persist);
bool cmd(const char *n, cb fun, const char *proto);
void execstring(const char *str, int isdown=1);
void execcfg(const char *path);
bool execfile(const char *path);
void resetcomplete(void);
void complete(string &s);
void setivar(const char *name, int i);
int getivar(const char *name);

// command with custom name
#define CMDN(name, fun, proto) \
  bool __##fun = q::script::cmd(#name, (q::script::cb) fun, proto)
// command with automatic name
#define CMD(name, proto) CMDN(name, name, proto)
// float persistent variable
#define FVARP(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, true)
// float non-persistent variable
#define FVAR(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, false)
// float non-persistent variable with specific name
#define FVARN(varname, name, min, cur, max) \
  float varname = q::script::fvar(#name, min, cur, max, &varname, NULL, false)
// float persistent variable with code to run when changed
#define FVARPF(name, min, cur, max, body) \
  void var_##name(); \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, true); \
  void var_##name() { body; } \
// float non-persistent variable with code to run when changed
#define FVARF(name, min, cur, max, body) \
  void var_##name(); \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, false); \
  void var_##name() { body; }
// int persistent variable
#define IVARP(name, min, cur, max) \
  int name = q::script::ivar(#name, min, cur, max, &name, NULL, true)
// int non-persistent variable
#define IVAR(name, min, cur, max) \
  int name = q::script::ivar(#name, min, cur, max, &name, NULL, false)
// int persistent variable with code to run when changed
#define IVARPF(name, min, cur, max, body) \
  void var_##name(); \
  int name = q::script::ivar(#name, min, cur, max, &name, var_##name, true); \
  void var_##name() { body; }
// int non-persistent variable with code to run when changed
#define IVARF(name, min, cur, max, body) \
  void var_##name(); \
  int name = q::script::ivar(#name, min, cur, max, &name, var_##name, false); \
  void var_##name() { body; }
} /* namespace script */
} /* namespace q */

