/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - script.cpp -> defines embedded script / console language
 -------------------------------------------------------------------------*/
#include "script.hpp"
#include "con.hpp"
#include "sys.hpp"

namespace q {
namespace script {

enum { FVAR, IVAR, FUN };
template <class T> struct cvar { T *storage,min,max; cb fun; bool persist; };
struct fn { const char *proto; cb fun; };
struct identifier {
  int type;
  union {
    cvar<int> ivar;
    cvar<float> fvar;
    fn fun;
  };
};
static hashtable<identifier> &idents() {
  static hashtable<identifier> internal;
  return internal;
}
int ivar(const char *n, int m, int cur, int M, int *ptr, cb fun, bool persist) {
  identifier v = {IVAR}; v.ivar = {ptr, m, M, fun, persist};
  idents().access(n, &v);
  return cur;
}
float fvar(const char *n, float m, float cur, float M, float *ptr, cb fun, bool persist) {
  identifier v = {FVAR}; v.fvar = {ptr, m, M, fun, persist};
  idents().access(n, &v);
  return cur;
}
bool cmd(const char *n, cb fun, const char *proto) {
  identifier v = {FUN}; v.fun = {proto, fun};
  idents().access(n, &v);
  return true;
}
void setivar(const char *name, int i)   {*idents().access(name)->ivar.storage = i;}
void setfvar(const char *name, float f) {*idents().access(name)->fvar.storage = f;}
int   getivar(const char *name) {return *idents().access(name)->ivar.storage;}
float getfvar(const char *name) {return *idents().access(name)->fvar.storage;}

typedef linear_allocator<1024> ctx;
static char *parseword(ctx &c, const char *&p) {
  p += strspn(p, " \t\r");
  if (p[0]=='/' && p[1]=='/') p += strcspn(p, "\n\0");
  if (*p=='[') { // nested string
    const auto left = *p++;
    const auto word = p;
    for (int brak = 1; brak; ) {
      const auto c = *p++;
      if (c==left) brak++;
      else if (c==']') brak--;
      else if (!c) { p--; con::out("missing \"]\""); return NULL; }
    }
    return c.newstring(word, p-word-1);
  }
  auto w = p;
  p += strcspn(p, "; \t\r\n\0");
  if (p-w==0) return c.newstring("");
  return c.newstring(w, p-w);
}

#define VAR(T,S) do {\
  const auto min = id->S##var.min, max = id->S##var.max;\
  auto &s = *id->S##var.storage;\
  if (!w[1][0]) con::out("%s = %" #S, c, s);\
  else {\
    if (min>max) con::out("variable is read-only");\
    else {\
      T i1 = ato##S(w[1]);\
      if (i1<min || i1>max) {\
        i1 = i1<min ? min : max;\
        con::out("range for %s is %"#S"..%"#S,c,min,max);\
      }\
      s = i1;\
    }\
    if (id->fun.fun) id->fun.fun();\
  }\
} while (0)

static void execute(ctx &c, const char *pp, int isdown) {
  auto p = pp;
  const int MAXWORDS = 32;
  char *w[MAXWORDS];

  // for each ; separated statement
  for (bool cont = true; cont;) {
    auto numargs = MAXWORDS;
    c.rewind();
    loopi(MAXWORDS) {
      w[i] = c.newstring("");
      if (i>numargs) continue;
      auto s = parseword(c,p);
      if (!s) numargs = i;
      w[i] = s;
    }
    loopi(MAXWORDS) if (w[i] == NULL) {
      con::out("out-of-memory for script execution");
      return;
    }

    p += strcspn(p, ";\n\0");
    cont = *p++!=0; // more statements if this isn't the end of the string
    auto ch = w[0];
    if (*ch=='/') ch++; // strip irc-style command prefix
    if (!*ch) continue; // empty statement

    auto id = idents().access(ch);
    if (!id) {
      con::out("unknown command: %s", ch);
      return;
    }

    switch (id->type) {
      case FUN: {
        const auto proto = id->fun.proto;
        const auto arity = strlen(proto);
        loopi(min(arity, numargs-1)) {
          if (proto[i]=='d') memcpy(w[i+1],&isdown,sizeof(isdown));
#define ARG(T,C,S) if(proto[i]==C){T x=ato##S(w[i]);memcpy(w[i],&x,sizeof(T));}
          ARG(int, 'i', i)
          ARG(float, 'f', f)
#undef ARG
        }
        switch (arity) {
#define C char*
#define PROTO(...) ((int (CDECL*)(__VA_ARGS__))id->fun.fun)
          case 0: PROTO()(); break;
          case 1: PROTO(C)(w[1]); break;
          case 2: PROTO(C,C)(w[1],w[2]); break;
          case 3: PROTO(C,C,C)(w[1],w[2],w[3]); break;
          case 4: PROTO(C,C,C,C)(w[1],w[2],w[3],w[4]); break;
#undef C
#undef PROTO
          default: break;
        }
      }
      break;
      case IVAR: VAR(int, i); break;
      case FVAR: VAR(float, f); break;
    }
  }
}
#undef VAR

// tab-completion of all idents
static int completesize = 0, completeidx = 0;
void resetcomplete(void) { completesize = 0; }

void complete(string &s) {
  if (*s!='/') sprintf_s(s)("/%s",s);
  if (!s[1]) return;
  if (!completesize) { completesize = int(strlen(s)-1); completeidx = 0; }
  int idx = 0;
  auto &t = idents();
  for (auto it = t.begin(); it != t.end(); ++it)
    if (strncmp(it->first, s+1, completesize)==0 && idx++==completeidx)
      sprintf_s(s)("/%s", it->first);
  completeidx++;
  if (completeidx>=idx) completeidx = 0;
}

void execstring(const char *str, int isdown) {ctx c; execute(c,str,isdown);}

bool execfile(const char *cfgfile) {
  sprintf_sd(s)("%s", cfgfile);
  char *buf = sys::loadfile(sys::path(s), NULL);
  if (!buf) return false;
  execstring(buf);
  free(buf);
  return true;
}

void execcfg(const char *cfgfile) {
  if (!execfile(cfgfile)) con::out("could not read \"%s\"", cfgfile);
}

void writecfg(void) {
  auto f = fopen("config.q", "w");
  if (!f) return;
  fprintf(f, "// automatically written on exit, do not modify\n"
             "// delete this file to have defaults.cfg overwrite these settings\n"
             "// modify settings in game, or put settings in autoexec.cfg to override anything\n\n");
  auto &t = idents();
  for (auto it = t.begin(); it != t.end(); ++it)
    if (it->second.type == IVAR && it->second.ivar.persist)
      fprintf(f, "%s %d\n", it->first, *it->second.ivar.storage);
    else if (it->second.type == FVAR && it->second.fvar.persist)
      fprintf(f, "%s %f\n", it->first, *it->second.fvar.storage);
  fclose(f);
}
CMD(writecfg, "");
} /* namespace script */
} /* namespace q */

