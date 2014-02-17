/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - task.hpp -> exposes multi-threaded tasking system
 -------------------------------------------------------------------------*/
#pragma once
#include "stl.hpp"
#include "sys.hpp"

namespace q {
class CACHE_LINE_ALIGNED task : public noncopyable, public refcount {
public:
  static void start(const u32 *queueinfo, u32 n);
  static void finish(void);
  task(const char *name, u32 elem=1, u32 waiter=0, u32 queue=0, u16 policy=0);
  virtual ~task(void);
  void starts(task&);
  void ends(task&);
  void wait(void);
  void scheduled(void);
  virtual void run(u32);
  static const u32 LO_PRIO = 0u;
  static const u32 HI_PRIO = 1u;
  static const u32 FAIR    = 0u;
  static const u32 UNFAIR  = 2u;
  static const u32 SIZE    = 192u;
  char DEFAULT_ALIGNED opaque[SIZE];
};
} /* namespace q */

