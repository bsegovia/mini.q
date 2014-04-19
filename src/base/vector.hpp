/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - vector.hpp -> exposes resizeable vector
 -------------------------------------------------------------------------*/
#pragma once
#include "base/sys.hpp"
#include "base/stl.hpp"
#include "base/utility.hpp"

namespace q {
template <class T> struct vector : noncopyable {
  T *buf;
  int alen, ulen;
  INLINE vector(int len = 0) {
    if (len) buf = (T*) MALLOC(len*sizeof(T)); else buf = NULL;
    loopi(len) sys::callctor<T>(buf+i);
    alen = ulen = len;
  }
  INLINE ~vector() { setsize(0); FREE(buf); }
  INLINE void destroy() { vector<T>().moveto(*this); }
  INLINE T &add(const T &x) {
    const T copy(x);
    if (ulen==alen) realloc();
    sys::callctor<T>(buf+ulen, copy);
    return buf[ulen++];
  }
  INLINE T &add() {
    if (ulen==alen) realloc();
    sys::callctor<T>(buf+ulen);
    return buf[ulen++];
  }
  pair<T*,u32> move() {
    const auto dst = makepair(buf,u32(ulen));
    alen = ulen = 0;
    buf = NULL;
    return dst;
  }
  INLINE void moveto(vector<T> &other) {
    other.~vector<T>();
    other.alen = alen; alen = 0;
    other.ulen = ulen; ulen = 0;
    other.buf = buf; buf = NULL;
  }
  INLINE void memset(u8 v) {::memset(buf, v, sizeof(T)*ulen);}
  T *begin() { return buf; }
  T *end() { return buf+ulen; }
  const T *begin() const { return buf; }
  const T *end() const { return buf+ulen; }
  INLINE T &pop() { return buf[--ulen]; }
  INLINE T &last() { return buf[ulen-1]; }
  INLINE bool empty() { return ulen==0; }
  INLINE int length() const { return ulen; }
  INLINE int size() const { return ulen*sizeof(T); }
  INLINE bool empty() const { return size()==0; }
  INLINE const T &operator[](int i) const { assert(i>=0 && i<ulen); return buf[i]; }
  INLINE T &operator[](int i) { assert(i>=0 && i<ulen); return buf[i]; }
  INLINE T *getbuf() { return buf; }
  INLINE void prealloc(int len) {
    if (len > alen) {
      alen = nextpowerof2(len);
      buf = (T*)REALLOC(buf, alen*sizeof(T));
    }
  }
  INLINE void realloc() {
    alen = 2*alen>1?2*alen:1;
    buf = (T*)REALLOC(buf, alen*sizeof(T));
  }

  void setsize(int i) {
    if (i<ulen)
      for(; ulen>i; --ulen) buf[ulen-1].~T();
    else if (i>ulen) {
      prealloc(i);
      for(; i>ulen; ++ulen) sys::callctor<T>(buf+ulen);
    }
  }

  T remove(int i) {
    assert(i<ulen);
    T e = buf[i];
    for(int p = i+1; p<ulen; p++) buf[p-1] = buf[p];
    ulen--;
    return e;
  }

  T removeunordered(int i) {
    assert(i<ulen);
    T e = buf[i];
    ulen--;
    if(ulen>0) buf[i] = buf[ulen];
    return e;
  }

  void removeobj(const T &o) {loopi(ulen) if(buf[i]==o) remove(i--);}
  T &insert(int i, const T &e) {
    add(T());
    for(int p = ulen-1; p>i; p--) buf[p] = buf[p-1];
    buf[i] = e;
    return buf[i];
  }

  // TODO remove this!
  void sort(void *cf) {
    qsort(buf, ulen, sizeof(T), (int (CDECL *)(const void *,const void *))cf);
  }

  static INLINE int heapparent(int i) { return (i - 1) >> 1; }
  static INLINE int heapchild(int i) { return (i << 1) + 1; }

  void buildheap() { for(int i = ulen/2; i >= 0; i--) downheap(i); }

  int upheap(int i) {
    auto score = buf[i];
    while (i > 0) {
      int pi = heapparent(i);
      if (!(score < buf[pi])) break;
      swap(buf[i], buf[pi]);
      i = pi;
    }
    return i;
  }

  T &addheap(const T &x) {
    add(x);
    return buf[upheap(ulen-1)];
  }

  int downheap(int i) {
    auto score = buf[i];
    for (;;) {
      int ci = heapchild(i);
      if (ci >= ulen) break;
      auto cscore = buf[ci];
      if (cscore < score) {
        if (ci+1 < ulen && buf[ci+1] < cscore) {
          swap(buf[ci+1], buf[i]);
          i = ci+1;
        } else {
          swap(buf[ci], buf[i]);
          i = ci;
        }
      } else if (ci+1 < ulen && buf[ci+1] < score) {
        swap(buf[ci+1], buf[i]);
        i = ci+1;
      } else
        break;
    }
    return i;
  }

  T removeheap() {
    T e = removeunordered(0);
    if (ulen) downheap(0);
    return e;
  }
};
typedef vector<char *> cvector;
typedef vector<int> ivector;
} /* namespace q */

