/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.cpp -> implements standard library routines
 -------------------------------------------------------------------------*/
#include "stl.hpp"

namespace q {

u32 murmurhash2(const void *key, int len, u32 seed) {
  const u32 m = 0x5bd1e995;
  const int r = 24;
  u32 h = seed ^ len;
  const u8* data = (const unsigned char *)key;
  while(len >= 4) {
    u32 k = *(u32 *)data;
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
    data += 4;
    len -= 4;
  }
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

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

intrusive_list_base::intrusive_list_base(void) : m_root() {}
intrusive_list_base::size_type intrusive_list_base::size(void) const {
  size_type numnodes(0);
  const intrusive_list_node* iter = &m_root;
  do {
    iter = iter->next;
    ++numnodes;
  } while (iter != &m_root);
  return numnodes - 1;
}
void append(intrusive_list_node *node, intrusive_list_node *prev) {
  assert(!node->in_list());
  node->next = prev->next;
  node->next->prev = node;
  prev->next = node;
  node->prev = prev;
}
void prepend(intrusive_list_node *node, intrusive_list_node *next) {
  assert(!node->in_list());
  node->prev = next->prev;
  node->prev->next = node;
  next->prev = node;
  node->next = next;
}
void link(intrusive_list_node* node, intrusive_list_node* nextNode) {
  prepend(node, nextNode);
}
void unlink(intrusive_list_node* node) {
  assert(node->in_list());
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = node->prev = node;
}
} /* namespace q */

