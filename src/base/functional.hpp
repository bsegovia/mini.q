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
#if defined(__MSVC__)
#pragma warning (push)
#pragma warning (disable : 4180)
#endif /* defined(__MSVC__) */

template <typename Result, typename... Args>
struct function<Result(Args...)> {
private:
  struct concept : q::refcount {
    virtual Result operator()(Args...) const = 0;
  };

  template <typename T>
  struct model : concept {
    model(const T &t) : callback(t) {}
    Result operator()(Args... a) const {
	  return callback(forward<Args>(a)...);
    }
    const T &callback;
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
#if defined(__MSVC__)
#pragma warning (pop)
#endif /* defined(__MSVC__) */
} /* namespace q */

