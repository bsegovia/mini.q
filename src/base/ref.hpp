/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - ref.hpp -> exposes reference counted objects
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "atomics.hpp"

namespace q {
template<typename T> struct ref {
  INLINE ref(void) : ptr(NULL) {}
  INLINE ref(const ref &input) : ptr(input.ptr) { if (ptr) ptr->acquire(); }
  INLINE ref(T* const input) : ptr(input) { if (ptr) ptr->acquire(); }
  INLINE ~ref(void) { if (ptr) ptr->release();  }
  INLINE operator bool(void) const       { return ptr != NULL; }
  INLINE operator T* (void)  const       { return ptr; }
  INLINE const T& operator* (void) const { return *ptr; }
  INLINE const T* operator->(void) const { return  ptr;}
  INLINE T& operator* (void) {return *ptr;}
  INLINE T* operator->(void) {return  ptr;}
  INLINE ref &operator= (const ref &input);
  INLINE ref &operator= (niltype);
  template<typename U> INLINE ref<U> cast(void) { return ref<U>((U*)(ptr)); }
  template<typename U> INLINE const ref<U> cast(void) const { return ref<U>((U*)(ptr)); }
  T* ptr;
};

struct refcount {
  INLINE refcount(void) : refcounter(0) {}
  virtual ~refcount(void) {}
  INLINE void acquire(void) { refcounter++; }
  INLINE void release(void) {
    if (--refcounter == 0) {
      auto ptr = this;
      DEL(ptr);
    }
  }
  atomic refcounter;
};

template <typename T>
INLINE ref<T> &ref<T>::operator= (const ref<T> &input) {
  if (input.ptr) input.ptr->acquire();
  if (ptr) ptr->release();
  *(T**)&ptr = input.ptr;
  return *this;
}
template <typename T>
INLINE ref<T> &ref<T>::operator= (niltype) {
  if (ptr) ptr->release();
  *(T**)&ptr = NULL;
  return *this;
}
} /* namespace q */

