/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.hpp -> exposes standard library routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "math.hpp"

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
  INLINE void advance(TIter& iter, TDist d, q::bidirectional_iterator_tag)
  {
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
} // namespace internal

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

// fast 32 bits murmur hash and its generic version
u32 murmurhash2(const void *key, int len, u32 seed = 0xffffffffu);
template <typename T> INLINE u32 murmurhash2(const T &x) { return murmurhash2(&x, sizeof(T)); }

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

/*-------------------------------------------------------------------------
 - pair
 -------------------------------------------------------------------------*/
template <typename T, typename U> struct pair {
  INLINE pair() {}
  INLINE pair(T t, U u) : first(t), second(u) {}
  T first; U second;
};
template <typename T, typename U>
INLINE bool operator==(const pair<T,U> &x0, const pair<T,U> &x1) {
  return x0.first == x1.first && x0.second == x1.second;
}
template <typename T, typename U>
INLINE pair<T,U> makepair(const T &t, const U &u){return pair<T,U>(t,u);}

/*-------------------------------------------------------------------------
 - very simple fixed size hash table (linear probing)
 -------------------------------------------------------------------------*/
template <typename T, u32 max_n=1024> struct hashtable : noncopyable {
  hashtable() : n(0) {}
  INLINE pair<const char*, T> *begin() { return items; }
  INLINE pair<const char*, T> *end() { return items+n; }
  INLINE void remove(pair<const char*, T> *it) {
    assert(it >= begin() && it < end());
    *it = items[(n--)-1];
  }
  T *access(const char *key, const T *value = NULL) {
    u32 h = 5381, i = 0;
    for (u32 k; (k = key[i]) != 0; ++i) h = ((h << 5) + h) ^ k;
    for (i = 0; i<n; ++i) if (h == hashes[i] && !strcmp(key, items[i].first)) break;
    if (value != NULL) {
      if (i>=max_n) sys::fatal("out-of-memory");
      items[i] = makepair(key,*value);
      hashes[i] = h;
      n = u32(i+1)>n?u32(i+1):n;
    }
    return i==n ? NULL : &items[i].second;
  }
  pair<const char*, T> items[max_n];
  u32 hashes[max_n], n;
};
} /* namespace q */

