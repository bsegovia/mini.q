/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - utility.hpp -> various utility functions and structures used by our stl
 - mostly taken from https://code.google.com/p/rdestl/
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include <new>

namespace q {

/*-------------------------------------------------------------------------
 - type traits
 -------------------------------------------------------------------------*/
template<typename T> struct is_integral { enum { value = false }; };
template<typename T> struct is_floating_point { enum { value = false }; };
template<int TVal> struct int_to_type { enum { value = TVal }; };

#define MAKE_INTEGRAL(TYPE) template<> struct is_integral<TYPE> { enum { value = true }; }
MAKE_INTEGRAL(char);
MAKE_INTEGRAL(unsigned char);
MAKE_INTEGRAL(bool);
MAKE_INTEGRAL(short);
MAKE_INTEGRAL(unsigned short);
MAKE_INTEGRAL(int);
MAKE_INTEGRAL(unsigned int);
MAKE_INTEGRAL(long);
MAKE_INTEGRAL(unsigned long);
MAKE_INTEGRAL(wchar_t);
#undef MAKE_INTEGRAL

static const struct noinitializetype {noinitializetype() {}} noinitialize;
template<> struct is_floating_point<float> { enum { value = true }; };
template<> struct is_floating_point<double> { enum { value = true }; };
template<typename T> struct is_pointer { enum { value = false }; };
template<typename T> struct is_pointer<T*> { enum { value = true }; };
template<typename T> struct is_pod { enum { value = false }; };
template<typename T> struct is_fundamental {
  enum {
    value = is_integral<T>::value || is_floating_point<T>::value
  };
};
template<typename T> struct has_trivial_constructor {
  enum {
    value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
  };
};
template<typename T> struct has_trivial_copy {
  enum {
    value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
  };
};

template<typename T> struct has_trivial_assign {
  enum {
    value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
  };
};
template<typename T> struct has_trivial_destructor {
  enum {
    value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
  };
};
template<typename T> struct has_cheap_compare {
  enum {
    value = has_trivial_copy<T>::value && sizeof(T) <= 4
  };
};
template<class T> struct isclass {
  template<class C> static char test(void (C::*)(void));
  template<class C> static int test(...);
  enum { yes = sizeof(test<T>(0)) == 1 ? 1 : 0, no = yes^1 };
};

namespace internal {
template<typename T>
void copy_n(const T* first, size_t n, T* result, int_to_type<false>) {
  const T* last = first + n;
  switch (n & 0x3) {
    case 0:
    while (first != last) {
      *result++ = *first++;
    case 3: *result++ = *first++;
    case 2: *result++ = *first++;
    case 1: *result++ = *first++;
    }
  }
}
template<typename T>
void copy_n(const T* first, size_t n, T* result, int_to_type<true>) {
  assert(result >= first + n || result < first);
  memcpy(result, first, n * sizeof(T));
}

template<typename T>
void copy(const T* first, const T* last, T* result, int_to_type<false>) {
  T *localResult = result;
  while (first != last)
    *localResult++ = *first++;
}
template<typename T>
void copy(const T* first, const T* last, T* result, int_to_type<true>) {
  const size_t n = reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first);
  memcpy(result, first, n);
}

template<typename T>
INLINE void move_n(const T* from, size_t n, T* result, int_to_type<false>) {
  for (int i = int(n) - 1; i >= 0; --i)
    result[i] = from[i];
}
template<typename T>
INLINE void move_n(const T* first, size_t n, T* result, int_to_type<true>) {
  memmove(result, first, n * sizeof(T));
}

template<typename T>
INLINE void move(const T* first, const T* last, T* result, int_to_type<false>) {
  result += (last - first);
  while (--last >= first)
    *(--result) = *last;
}

template<typename T>
INLINE void move(const T* first, const T* last, T* result, int_to_type<true>) {
  const size_t n = reinterpret_cast<uintptr>(last) - reinterpret_cast<uintptr>(first);
  memmove(result, first, n);
}

template<typename T>
void copy_construct_n(const T* first, size_t n, T* result, int_to_type<false>) {
  for (size_t i = 0; i < n; ++i)
    new (result + i) T(first[i]);
}
template<typename T>
void copy_construct_n(const T* first, size_t n, T* result, int_to_type<true>) {
  assert(result >= first + n || result < first);
  memcpy(result, first, n * sizeof(T));
}

template<typename T>
void destruct_n(T* first, size_t n, int_to_type<false>) {
  for (size_t i = 0; i < n; ++i)
    (first + i)->~T();
}
template<typename T>
INLINE void destruct_n(T*, size_t, int_to_type<true>) {}

template<typename T>
INLINE void destruct(T* mem, int_to_type<false>) { mem->~T(); }
template<typename T>
INLINE void destruct(T*, int_to_type<true>) {}

template<typename T>
void construct(T* mem, int_to_type<false>) { new (mem) T(); }
template<typename T>
INLINE void construct(T*, int_to_type<true>) {}

template<typename T>
INLINE void copy_construct(T* mem, const T& orig, int_to_type<false>) {
  new (mem) T(orig);
}
template<typename T>
INLINE void copy_construct(T* mem, const T& orig, int_to_type<true>) {
  mem[0] = orig;
}

template<typename T>
void construct_n(T* to, size_t count, int_to_type<false>) {
  for (size_t i = 0; i < count; ++i)
    new (to + i) T();
}
template<typename T>
INLINE void construct_n(T*, int, int_to_type<true>) {}

// tests if all elements in range are ordered according to pred.
template<class TIter, class TPred>
#if STL_DEBUG
void test_ordering(TIter first, TIter last, const TPred& pred) {
  if (first != last) {
    TIter next = first;
    if (++next != last) {
      assert(pred(*first, *next));
      first = next;
    }
  }
}
#else
void test_ordering(TIter first, TIter last, const TPred& pred) {}
#endif // STL_DEBUG

template<typename T1, typename T2, class TPred> inline
bool debug_pred(const TPred& pred, const T1& a, const T2& b) {
#if STL_DEBUG
  if (pred(a, b)) {
    assert(!pred(b, a));
    return true;
  } else
    return false;
#else
  return pred(a, b);
#endif // STL_DEBUG
}
} /* namespace internal */
/*-------------------------------------------------------------------------
 - iterators
 -------------------------------------------------------------------------*/
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag: public input_iterator_tag {};
struct bidirectional_iterator_tag: public forward_iterator_tag {};
struct random_access_iterator_tag: public bidirectional_iterator_tag {};

template<typename IterT> struct iterator_traits {
   typedef typename IterT::iterator_category iterator_category;
};
template<typename T> struct iterator_traits<T*> {
   typedef random_access_iterator_tag iterator_category;
};

namespace internal {
template<typename TIter, typename TDist>
INLINE void distance(TIter first, TIter last, TDist& dist, q::random_access_iterator_tag) {
  dist = TDist(last - first);
}
template<typename TIter, typename TDist>
INLINE void distance(TIter first, TIter last, TDist& dist, q::input_iterator_tag) {
  dist = 0;
  while (first != last) {
    ++dist;
    ++first;
  }
}
template<typename TIter, typename TDist>
INLINE void advance(TIter& iter, TDist d, q::random_access_iterator_tag) {
  iter += d;
}
template<typename TIter, typename TDist>
INLINE void advance(TIter& iter, TDist d, q::bidirectional_iterator_tag) {
  if (d >= 0)
    while (d--) ++iter;
  else
    while (d++) --iter;
}
template<typename TIter, typename TDist>
INLINE void advance(TIter& iter, TDist d, q::input_iterator_tag) {
  assert(d >= 0);
  while (d--) ++iter;
}
} /* namespace internal */

// get the power larger or equal than x
INLINE u32 nextpowerof2(u32 x) {
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
INLINE bool ispoweroftwo(u32 x) { return ((x&(x-1))==0); }
INLINE u32 ilog2(u32 x) {
  u32 l = 0;
  while (x >>= 1) ++l;
  return l;
}

template<typename T>
struct less {
  INLINE bool operator()(const T& lhs, const T& rhs) const {return lhs < rhs;}
};

template<typename T>
struct greater {
  INLINE bool operator()(const T& lhs, const T& rhs) const {return lhs > rhs;}
};

template<typename T>
struct equal_to {
  INLINE bool operator()(const T& lhs, const T& rhs) const {return lhs == rhs;}
};

// global variable with proper constructor
#define GLOBAL(TYPE, NAME) static TYPE &NAME() {static TYPE var; return var;}
template <typename T> void swap(T &x, T &y) { T tmp = x; x = y; y = tmp; }

// makes a class non-copyable
class noncopyable {
protected:
  INLINE noncopyable() {}
  INLINE ~noncopyable() {}
private:
  INLINE noncopyable(const noncopyable&) {}
  INLINE noncopyable& operator= (const noncopyable&) {return *this;}
};
} /* namespace q */

