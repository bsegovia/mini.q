/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - functional.hpp -> implements some of std functional
 -------------------------------------------------------------------------*/
#pragma once
#include "sys.hpp"
#include "ref.hpp"
#include <type_traits>

namespace q {

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

template <typename T>
struct function;

template <typename Result, typename... Args>
struct function<Result(Args...)> {
private:
  struct concept : q::refcount {
    virtual Result operator()(Args...) const = 0;
  };

  template <typename T>
  struct model : concept {
    model(const T &t) : t(t) {}
    Result operator()(Args... a) const {
      return t(forward<Args>(a)...);
    }
    const T &t;
  };

  q::ref<concept> fn;

public:
  INLINE function() {}
  template <typename T>
  function(const T &t) : fn(NEW(model<T>, t)) {}
  Result operator()(Args... args) const {
    return (*fn)(forward<Args>(args)...);
  }
};
} /* namespace q */

