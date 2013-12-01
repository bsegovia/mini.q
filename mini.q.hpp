/*-------------------------------------------------------------------------
 - game
 -------------------------------------------------------------------------*/
namespace game {
extern float lastmillis, speed, curtime;
} /* namespace game */

/*-------------------------------------------------------------------------
 - console
 -------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 - simple script system mostly compatible with quake console
 -------------------------------------------------------------------------*/
namespace script {
typedef void (CDECL *cb)();
int ivar(const char *n, int m, int cur, int M, int *ptr, cb fun, bool persist);
float fvar(const char *n, float m, float cur, float M, float *ptr, cb fun, bool persist);
bool cmd(const char *n, cb fun, const char *proto);
void execute(const char *str, int isdown=1);

// command with custom name
#define CMDN(name, fun, proto) \
  static bool __##fun = q::script::cmd(#name, (q::script::cb) fun, proto)
// command with automatic name
#define CMD(name, proto) CMDN(name, name, proto)
// float persistent variable
#define FVARP(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, true)
// float non-persistent variable
#define FVAR(name, min, cur, max) \
  float name = q::script::fvar(#name, min, cur, max, &name, NULL, false)
// float persistent variable with code to run when changed
#define FVARPF(name, min, cur, max, body) \
  void var_##name() { body; } \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, true);
// float non-persistent variable with code to run when changed
#define FVARF(name, min, cur, max, body) \
  void var_##name() { body; } \
  float name = q::script::fvar(#name, min, cur, max, &name, var_##name, false);
// int persistent variable
#define IVARP(name, min, cur, max) \
  auto name = q::script::ivar(#name, min, cur, max, &name, NULL, true)
// int non-persistent variable
#define IVAR(name, min, cur, max) \
  auto name = q::script::ivar(#name, min, cur, max, &name, NULL, false)
// int persistent variable with code to run when changed
#define IVARPF(name, min, cur, max, body) \
  void var_##name() { body; } \
// int non-persistent variable with code to run when changed
#define IVARF(name, min, cur, max, body) \
  void var_##name() { body; } \
  auto name = q::script::ivar(#name, min, cur, max, &name, var_##name, false);

} /* namespace script */

/*-------------------------------------------------------------------------
 - opengl interface
 -------------------------------------------------------------------------*/
namespace ogl {

} /* namespace ogl */
} /* namespace q */

