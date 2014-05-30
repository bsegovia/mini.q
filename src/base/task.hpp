/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - task.hpp -> exposes multi-threaded tasking system
 -------------------------------------------------------------------------*/
#pragma once
#include "atomics.hpp"
#include "ref.hpp"
#include "intrusive_list.hpp"
#include "sys.hpp"

namespace q {
namespace tasking {
struct queue;
} /* namespace tasking */

class CACHE_LINE_ALIGNED task : public noncopyable, public intrusive_list_node, public refcount {
public:
  static void start(const u32 *queueinfo, u32 n);
  static void finish(void);
  task(const char *name, u32 elem=1, u32 waiter=0, u32 queue=0, u16 policy=0);
  virtual ~task(void);
  virtual void run(u32) = 0;
  void starts(task&);
  void ends(task&);
  void scheduled(void);
  void wait(bool recursivewait=false);
  static const u32 LO_PRIO = 0u;
  static const u32 HI_PRIO = 1u;
  static const u32 FAIR    = 0u;
  static const u32 UNFAIR  = 2u;
  static const u32 MAXSTART = 4; // maximum number of tasks we may start
  static const u32 MAXEND   = 4; // maximum number of tasks we may end
  static const u32 MAXDEP   = 8; // maximum number of tasks we may depend on
private:
  friend tasking::queue;
  task *taskstostart[MAXSTART];// all the tasks that wait for us to start
  task *taskstoend[MAXEND];    // all the tasks that wait for us to finish
  task *deps[MAXDEP];          // all the tasks we depend on to finish or start
  tasking::queue *const owner; // where the task runs when ready
  const char *name;            // name of the task (may be NULL)
  atomic elemnum;              // number of items still to run in the set
  atomic tostart;              // mbz to start
  atomic toend;                // mbz to end
  atomic depnum;               // number of tasks we depend to finish
  atomic waiternum;            // number of wait() that still need to be done
  atomic tasktostartnum;       // number of tasks we need to start
  atomic tasktoendnum;         // number of tasks we need to end
  const u16 policy;            // handle fairness and priority
  volatile u16 state;          // track task state (useful to debug)
};
} /* namespace q */

