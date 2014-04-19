/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - string.cpp -> implements string class and routines
 -------------------------------------------------------------------------*/
#include "string.hpp"

namespace q {
char *newstring(const char *s, size_t l, const char *filename, int linenum) {
  char *b = (char*) sys::memalloc(l+1, filename, linenum);
  if (s) strncpy(b,s,l);
  b[l] = 0;
  return b;
}
char *newstring(const char *s, const char *filename, int linenum) {
  return newstring(s, strlen(s), filename, linenum);
}
char *newstringbuf(const char *s, const char *filename, int linenum) {
  return newstring(s, MAXDEFSTR-1, filename, linenum);
}

char *tokenize(char *s1, const char *s2, char **lasts) {
 char *ret;
 if (s1 == NULL)
   s1 = *lasts;
 while(*s1 && strchr(s2, *s1))
   ++s1;
 if(*s1 == '\0')
   return NULL;
 ret = s1;
 while(*s1 && !strchr(s2, *s1))
   ++s1;
 if(*s1)
   *s1++ = '\0';
 *lasts = s1;
 return ret;
}

bool strequal(const char *s1, const char *s2) {
  if (strcmp(s1, s2) == 0) return true;
  return false;
}

bool contains(const char *haystack, const char *needle) {
  if (strstr(haystack, needle) == NULL) return false;
  return true;
}

void fixedstring::fmt(const char *txt, va_list va) {
 vsnprintf(bytes, MAXDEFSTR, txt, va);
 bytes[MAXDEFSTR-1] = 0;
}

void fixedstring::fmt(const char *txt, ...) {
  va_list va;
  va_start(va, txt);
  fmt(txt, va);
  va_end(va);
}

fixedstring::fixedstring(const char *txt, va_list va) { fmt(txt, va); }
fixedstring::fixedstring(fmttype, const char *txt, ...) {
  va_list va;
  va_start(va, txt);
  fmt(txt, va);
  va_end(va);
}
} /* namespace q */

