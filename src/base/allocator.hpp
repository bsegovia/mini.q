/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.hpp -> exposes standard library routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"

namespace q {

/*-------------------------------------------------------------------------
 - fast growing pool allocation
 -------------------------------------------------------------------------*/
template <typename T> class growingpool {
public:
  growingpool(void) : current(NEW(growingpoolelem, 1)) {}
  ~growingpool(void) { SAFE_DEL(current); }
  T *allocate(void) {
    if (current->allocated == current->maxelemnum) {
      growingpoolelem *elem = NEW(growingpoolelem, 2*current->maxelemnum);
      elem->next = current;
      current = elem;
    }
    T *data = current->data + current->allocated++;
    return data;
  }
private:
  // chunk of elements to allocate
  struct growingpoolelem {
    growingpoolelem(size_t elemnum) {
      data = NEWAE(T, elemnum);
      next = NULL;
      maxelemnum = elemnum;
      allocated = 0;
    }
    ~growingpoolelem(void) {
      SAFE_DELA(data);
      SAFE_DEL(next);
    }
    T *data;
    growingpoolelem *next;
    size_t allocated, maxelemnum;
  };
  growingpoolelem *current;
};

// very simple fixed size circular buffer that wraps around
template <typename T, u32 max_n> struct DEFAULT_ALIGNED ringbuffer {
  INLINE ringbuffer() : first(0x7fffffff), n(0) {}
  INLINE T &back()  {return items[(first+n-1)%max_n];}
  INLINE T &front() {return items[first%max_n];}
  INLINE void popfront() {if (n!=0) {first++;--n;}}
  INLINE void popback()  {if (n!=0) {--n;}}
  INLINE T &addback() {
    if (n==max_n) popfront(); ++n;
    return items[(first+n-1)%max_n];
  }
  INLINE T &addfront() {
    if (n==max_n) popback(); ++n;
    return items[--first%max_n];
  }
  INLINE T &operator[](u32 idx) { return items[(first+idx)%max_n]; }
  INLINE u32 size() const {return n;}
  INLINE bool empty() const {return n==0;}
  INLINE bool full() const {return n==max_n;}
  T items[max_n];
  u32 first, n;
};

// very simple fixed size linear allocator
template <u32 size, u32 align=sizeof(int)>
struct DEFAULT_ALIGNED linear_allocator {
  INLINE linear_allocator() : head(data) {}
  INLINE char *newstring(const char *s, u32 l=~0u);
  INLINE void rewind() { head=data; }
  INLINE char *alloc(int sz) {
    if (head+ALIGN(sz,align)>data+size) sys::fatal("out-of-memory");
    head += ALIGN(sz,align);
    return head-sz;
  }
  char *head, data[size];
};

/*-------------------------------------------------------------------------
 - allocator
 -------------------------------------------------------------------------*/
class allocator {
public:
  explicit allocator(const char* name = "DEFAULT"):  m_name(name) {}
  ~allocator(void) {}
  void *allocate(u32 bytes, int flags = 0);
  void *allocate_aligned(u32 bytes, u32 alignment, int flags = 0);
  void deallocate(void* ptr, u32 bytes);
  const char *get_name(void) const { return m_name; }
private:
  const char *m_name;
};

INLINE bool operator==(const allocator&, const allocator&) {
  return true;
}
INLINE bool operator!=(const allocator& lhs, const allocator& rhs) {
  return !(lhs == rhs);
}
INLINE void* allocator::allocate(u32 bytes, int) {
  return sys::memalloc(bytes, __FILE__, __LINE__);
}
INLINE void allocator::deallocate(void* ptr, u32) {
  sys::memfree(ptr);
}
} /* namespace q */

#include "string.hpp"
namespace q {
template <u32 size, u32 align>
INLINE char *linear_allocator<size,align>::newstring(const char *s, u32 l) {
  if (l==~0u) l=strlen(s);
  auto d = strncpy(alloc(l+1),s,l); d[l]=0;
  return d;
}
} /* namespace q */

