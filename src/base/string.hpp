/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - string.hpp -> exposes string classes and routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"

namespace q {

// more string operations
INLINE char *strn0cpy(char *d, const char *s, int m) {strncpy(d,s,m); d[m-1]=0; return d;}

// easy safe string
static const u32 MAXDEFSTR = 260, WORDWRAP = 80;
struct fixedstring {
  INLINE fixedstring();
  INLINE fixedstring(const char *txt);
#if 0
  char *operator[] (int idx) { return bytes[idx]; }
  const char *operator[] (int idx) const { return bytes[idx]; }
  const char *c_str() const { return &bytes[0]; }
  char *c_str() { return &bytes[0]; }
#else
  operator const char *() const { return bytes; }
  operator char *() { return bytes; }
#endif
  char bytes[MAXDEFSTR];
};
INLINE void strcpy_cs(char *d, const char *s) { strn0cpy(d,s,MAXDEFSTR); }
INLINE void strcat_cs(char *d, const char *s) { size_t n = strlen(d); strn0cpy(d+n,s,MAXDEFSTR-n); }
INLINE void strcpy_s(fixedstring &d, const char *s) { strcpy_cs(d,s); }
INLINE void strcat_s(fixedstring &d, const char *s) { strcat_cs(d,s); }
INLINE void strfmt_s(fixedstring &d, const char *fmt, va_list v) {
  vsnprintf(d, MAXDEFSTR, fmt, v);
  d[MAXDEFSTR-1] = 0;
}
INLINE fixedstring::fixedstring() { bytes[0] = '\0'; }
INLINE fixedstring::fixedstring(const char *txt) { strcpy_cs(bytes,txt); }
char *newstring(const char *s, const char *filename, int linenum);
char *newstring(const char *s, size_t sz, const char *filename, int linenum);
char *newstringbuf(const char *s, const char *filename, int linenum);
#define NEWSTRING(...) newstring(__VA_ARGS__, __FILE__, __LINE__)
#define NEWSTRINGBUF(S) newstringbuf(S, __FILE__, __LINE__)

char *tokenize(char *s1, const char *s2, char **lasts);
bool strequal(const char *s1, const char *s2);
bool contains(const char *haystack, const char *needle);

// format string boilerplate
struct sprintfmt_s {
  INLINE sprintfmt_s(fixedstring &str) : d(str) {}
  void operator()(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    vsnprintf(d, MAXDEFSTR, fmt, v);
    va_end(v);
    d[MAXDEFSTR-1] = 0;
  }
  static INLINE sprintfmt_s make(fixedstring &s) { return sprintfmt_s(s); }
  fixedstring &d;
};
#define sprintf_s(d) sprintfmt_s::make(d)
#define sprintf_sd(d) fixedstring d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) fixedstring d; {va_list ap; va_start(ap,last); strfmt_s(d,fmt,ap); va_end(ap);}
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)
#define ATOI(s) strtol(s, NULL, 0) // supports hexadecimal numbers
} /* namespace q */

