/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - stl.hpp -> exposes standard library routines
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "math.hpp"

namespace q {

/*-------------------------------------------------------------------------
 - pair
 -------------------------------------------------------------------------*/
template <typename T, typename U> struct pair {
  INLINE pair() {}
  INLINE pair(T t, U u) : first(t), second(u) {}
  template <typename T0, typename U0>
  INLINE pair(const pair<T0, U0> &other) :
    first(T(other.first)), second(U(other.second)) {}
  T first; U second;
};
template <typename T, typename U>
INLINE bool operator==(const pair<T,U> &x0, const pair<T,U> &x1) {
  return x0.first == x1.first && x0.second == x1.second;
}
template <typename T, typename U>
INLINE pair<T,U> makepair(const T &t, const U &u){return pair<T,U>(t,u);}
} /* namespace q */

