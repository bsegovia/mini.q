/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - algorithm.hpp -> various stl like algorithms
 -------------------------------------------------------------------------*/
#pragma once
#include "utility.hpp"

namespace q {
template<typename T>
INLINE void copy_construct(T* mem, const T& orig) {
  internal::copy_construct(mem, orig, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T>
INLINE void construct(T* mem) {
  internal::construct(mem, int_to_type<has_trivial_constructor<T>::value>());
}

template<typename T>
INLINE void destruct(T* mem) {
  internal::destruct(mem, int_to_type<has_trivial_destructor<T>::value>());
}

template<typename T>
void copy_n(const T* first, size_t n, T* result) {
  internal::copy_n(first, n, result, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T>
void copy(const T* first, const T* last, T* result) {
  internal::copy(first, last, result, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T> void copy_construct_n(T* first, size_t n, T* result) {
  internal::copy_construct_n(first, n, result, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T> void move_n(const T* from, size_t n, T* result) {
  assert(from != result || n == 0);
  if (result + n >= from && result < from + n)
    internal::move_n(from, n, result, int_to_type<has_trivial_copy<T>::value>());
  else
    internal::copy_n(from, n, result, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T>
INLINE void move(const T* first, const T* last, T* result) {
  assert(first != result || first == last);
  const auto n = reinterpret_cast<uintptr>(last) - reinterpret_cast<uintptr>(first);
  const auto resultEnd = reinterpret_cast<const unsigned char*>(result) + n;
  if (resultEnd >= reinterpret_cast<const unsigned char*>(first) && result < last)
    internal::move(first, last, result, int_to_type<has_trivial_copy<T>::value>());
  else
    internal::copy(first, last, result, int_to_type<has_trivial_copy<T>::value>());
}

template<typename T>
void construct_n(T* first, size_t n) {
  internal::construct_n(first, n, int_to_type<has_trivial_constructor<T>::value>());
}

template<typename T>
void destruct_n(T* first, size_t n) {
  internal::destruct_n(first, n, int_to_type<has_trivial_destructor<T>::value>());
}

template<typename T>
INLINE void fill_n(T* first, size_t n, const T& val) {
  T* last = first + n;
  switch (n & 0x7) {
    case 0:
    while (first != last) {
      *first = val; ++first;
    case 7: *first = val; ++first;
    case 6: *first = val; ++first;
    case 5: *first = val; ++first;
    case 4: *first = val; ++first;
    case 3: *first = val; ++first;
    case 2: *first = val; ++first;
    case 1: *first = val; ++first;
    }
  }
}

template<typename TIter, typename TDist>
INLINE void distance(TIter first, TIter last, TDist& dist) {
  internal::distance(first, last, dist, typename iterator_traits<TIter>::iterator_category());
}

template<typename TIter, typename TDist> inline
void advance(TIter& iter, TDist off) {
  internal::advance(iter, off, typename iterator_traits<TIter>::iterator_category());
}

template<class TIter, typename T, class TPred> inline
TIter lower_bound(TIter first, TIter last, const T& val, const TPred& pred) {
  internal::test_ordering(first, last, pred);
  int dist(0);
  distance(first, last, dist);
  while (dist > 0) {
    const int halfDist = dist >> 1;
    TIter mid = first;
    advance(mid, halfDist);
    if (internal::debug_pred(pred, *mid, val))
      first = ++mid, dist -= halfDist + 1;
    else
      dist = halfDist;
  }
  return first;
}

template<class TIter, typename T, class TPred>
INLINE TIter upper_bound(TIter first, TIter last, const T& val, const TPred& pred) {
  internal::test_ordering(first, last, pred);
  int dist(0);
  distance(first, last, dist);
  while (dist > 0) {
    const int halfDist = dist >> 1;
    TIter mid = first;
    advance(mid, halfDist);
    if (!internal::debug_pred(pred, val, *mid))
      first = ++mid, dist -= halfDist + 1;
    else
      dist = halfDist;
  }
  return first;
}

template<class TIter, typename T>
TIter find(TIter first, TIter last, const T& val) {
  while (first != last) {
    if ((*first) == val) return first;
    ++first;
  }
  return last;
}

template<class TIter, typename T, class TPred>
TIter find_if(TIter first, TIter last, const T& val, const TPred& pred) {
  while (first != last) {
    if (pred(*first, val)) return first;
    ++first;
  }
  return last;
}

template<class TIter, typename T>
void accumulate(TIter first, TIter last, T& result) {
  while (first != last) {
    result += *first;
    ++first;
  }
}

template<class T> INLINE bool compareless(const T &x, const T &y) { return x < y; }
template<class T, class F>
INLINE void insertionsort(T *start, T *end, F fun) {
  for(T *i = start+1; i < end; i++) {
    if(fun(*i, i[-1])) {
      T tmp = *i;
      *i = i[-1];
      T *j = i-1;
      for(; j > start && fun(tmp, j[-1]); --j)
        *j = j[-1];
      *j = tmp;
    }
  }
}

template<class T, class F>
INLINE void insertionsort(T *buf, int n, F fun) {
  insertionsort(buf, buf+n, fun);
}

template<class T>
INLINE void insertionsort(T *buf, int n) {
  insertionsort(buf, buf+n, compareless<T>);
}

template<class T, class F> void quicksort(T *start, T *end, F fun) {
  while(end-start > 10) {
    T *mid = &start[(end-start)/2], *i = start+1, *j = end-2, pivot;
    if(fun(*start, *mid)) { // start < mid 
      if(fun(end[-1], *start)) { pivot = *start; *start = end[-1]; end[-1] = *mid; } // end < start < mid 
      else if(fun(end[-1], *mid)) { pivot = end[-1]; end[-1] = *mid; } // start <= end < mid 
      else { pivot = *mid; } // start < mid <= end
    }
    else if(fun(*start, end[-1])) { pivot = *start; *start = *mid; } // mid <= start < end 
    else if(fun(*mid, end[-1])) { pivot = end[-1]; end[-1] = *start; *start = *mid; } //  mid < end <= start 
    else { pivot = *mid; swap(*start, end[-1]); }  // end <= mid <= start 
    *mid = end[-2];
    do {
      while(fun(*i, pivot)) if(++i >= j) goto partitioned;
      while(fun(pivot, *--j)) if(i >= j) goto partitioned;
      swap(*i, *j);
    } while(++i < j);
partitioned:
    end[-2] = *i;
    *i = pivot;

    if(i-start < end-(i+1)) {
      quicksort(start, i, fun);
      start = i+1;
    } else {
      quicksort(i+1, end, fun);
      end = i;
    }
  }

  insertionsort(start, end, fun);
}

template<class T, class F> INLINE void quicksort(T *buf, int n, F fun) {
  quicksort(buf, buf+n, fun);
}

template<class T> INLINE void quicksort(T *buf, int n) {
  quicksort(buf, buf+n, compareless<T>);
}
} /* namespace q */

