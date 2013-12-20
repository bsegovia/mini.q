#pragma once
#include "sys.hpp"

/*-------------------------------------------------------------------------
 - tasking system: supports waitable tasks with input/output dependencies
 -------------------------------------------------------------------------*/
namespace q {
namespace tasking {
  void init(const u32 *queueinfo, u32 n);
  void clean(void);
} /* namespace tasking */

class CACHE_LINE_ALIGNED task : public noncopyable, public refcount {
public:
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

