/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - task.hpp -> implements multi-threaded tasking system
 -------------------------------------------------------------------------*/
#include "task.hpp"
#include <SDL/SDL_thread.h>

namespace q {
namespace tasking {

// track state of the task
enum {
  UNSCHEDULED = 0,
  SCHEDULED = 1,
  RUNNING = 2,
  DONE = 3
};

// all queues as instantiated by the user
static vector<struct queue*> queues;

static const u32 MAXSTART = 4; // maximum number of tasks we may start
static const u32 MAXEND   = 4; // maximum number of tasks we may end
static const u32 MAXDEP   = 8; // maximum number of tasks we may depend on

// internal hidden structure of the task
struct MAYALIAS internal : public noncopyable, public intrusive_list_node {
  INLINE internal(const char *name, u32 n, u32 waiternum, u32 queue, u16 policy);
  INLINE task *parent(void);
  void wait(bool recursivewait);
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
static_assert(sizeof(internal) <= task::SIZE, "opaque storage is too small");
static_assert(sizeof(task) == 256, "invalid task size");

// a set of threads subscribes this queue and share the work in the list of the
// tasks. threads terminate when "terminatethreads" become true
struct queue {
  queue(u32 threadnum);
  ~queue(void);
  void append(task*);
  void terminate(task*);
  static int threadfunc(void*);
  SDL_cond *cond;
  SDL_mutex *mutex;
  vector<SDL_Thread*> threads;
  intrusive_list<internal> readylist;
  volatile bool terminatethreads;
};

INLINE internal::internal(const char *name, u32 n, u32 waiternum, u32 queue, u16 policy) :
  owner(tasking::queues[queue]), name(name), elemnum(n), tostart(1), toend(n),
  depnum(0), waiternum(waiternum), tasktostartnum(0), tasktoendnum(0),
  policy(policy), state(tasking::UNSCHEDULED)
{}
INLINE task *internal::parent(void) {
  return (task*)((char*)this-OFFSETOF(task,opaque));
}
INLINE internal &inner(task *job) { return *(internal*)job->opaque; }

void internal::wait(bool recursivewait) {
  assert((recursivewait||state >= tasking::SCHEDULED) &&
         (recursivewait||waiternum > 0));
  auto job = parent();

  // execute all starting dependencies
  while (tostart)
    loopi(depnum)
      if (tasking::inner(deps[i]).toend)
        inner(deps[i]).wait(true);

  // execute the run function
  for (;;) {
    const auto elt = --elemnum;
    if (elt == 0) {
      SDL_LockMutex(owner->mutex);
        owner->readylist.pop_front();
      SDL_UnlockMutex(owner->mutex);
    }
    if (elt >= 0) {
      job->run(elt);
      if (--toend == 0) owner->terminate(job);
    }
    if (elt <= 0) break;
  }

  // execute all ending dependencies
  while (toend)
    loopi(depnum)
      if (tasking::inner(deps[i]).toend)
        inner(deps[i]).wait(true);

  // finished and no more waiters, we can safely release the dependency array
  if (!recursivewait && --waiternum == 0)
    loopi(depnum) deps[i]->release();
}

void queue::append(task *job) {
  auto &self = inner(job);
  assert(self.owner == this && self.tostart == 0);
  SDL_LockMutex(self.owner->mutex);
    if (self.policy & task::HI_PRIO)
      self.owner->readylist.push_front(&self);
    else
      self.owner->readylist.push_back(&self);
  SDL_CondBroadcast(cond);
  SDL_UnlockMutex(self.owner->mutex);
}

void queue::terminate(task *job) {
  auto &self = inner(job);
  assert(self.owner == this && self.toend == 0);
  storerelease(&self.state, u16(DONE));

  // go over all tasks that depend on us
  loopi(self.tasktostartnum) {
    if (--inner(self.taskstostart[i]).tostart == 0)
      append(self.taskstostart[i]);
    self.taskstostart[i]->release();
  }
  loopi(self.tasktoendnum) {
    if (--inner(self.taskstoend[i]).toend == 0)
      terminate(self.taskstoend[i]);
    self.taskstoend[i]->release();
  }

  // if no more waiters, we can safely free all dependencies since we are done
  if (self.waiternum == 0)
    loopi(self.depnum) self.deps[i]->release();
  job->release();
}

int queue::threadfunc(void *data) {
  auto q = (queue*) data;
  for (;;) {
    SDL_LockMutex(q->mutex);
    while (q->readylist.empty() && !q->terminatethreads)
      SDL_CondWait(q->cond, q->mutex);
    if (q->terminatethreads) {
      SDL_UnlockMutex(q->mutex);
      break;
    }
    auto &self = *q->readylist.front();
    auto job = self.parent();

    // if unfair, we run all elements until there is nothing else to do in this
    // job. we do not care if a hi-prio job arrives while we run
    if (self.policy & task::UNFAIR) {
      job->acquire();
      SDL_UnlockMutex(q->mutex);
      for (;;) {
        const auto elt = --self.elemnum;
        if (elt >= 0) {
          job->run(elt);
          if (--self.toend == 0) q->terminate(job);
        }
        if (elt == 0) {
          SDL_LockMutex(q->mutex);
            q->readylist.pop_front();
          SDL_UnlockMutex(q->mutex);
        }
        if (elt <= 0) break;
      }
      job->release();
    }
    // if fair, we run once and go back to the queue to possibly run something
    // with hi-prio that just arrived
    else {
      const auto elt = --self.elemnum;
      if (elt == 0) q->readylist.pop_front();
      SDL_UnlockMutex(q->mutex);
      if (elt >= 0) {
        job->run(elt);
        if (--self.toend == 0) q->terminate(job);
      }
    }
  }

  return 0;
}

queue::queue(u32 threadnum) : terminatethreads(false) {
  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
  loopi(s32(threadnum)) threads.add(SDL_CreateThread(threadfunc, this));
}

queue::~queue(void) {
  SDL_LockMutex(mutex);
  terminatethreads = true;
  SDL_CondBroadcast(cond);
  SDL_UnlockMutex(mutex);
  loopv(threads) SDL_WaitThread(threads[i], NULL);
  SDL_DestroyMutex(mutex);
  SDL_DestroyCond(cond);
}
} /* namespace tasking */


void task::start(const u32 *queueinfo, u32 n) {
  vector<tasking::queue*>(n).moveto(tasking::queues);
  loopi(s32(n)) tasking::queues[i] = NEW(tasking::queue, queueinfo[i]);
}

void task::finish(void) {
  loopv(tasking::queues) DEL(tasking::queues[i]);
  vector<tasking::queue*>().moveto(tasking::queues);
}

task::task(const char *name, u32 n, u32 waiternum, u32 queue, u16 policy) {
  new (opaque) tasking::internal(name,n,waiternum,queue,policy);
}
task::~task(void) { tasking::inner(this).~internal(); }

void task::run(u32 elt) {}

void task::scheduled(void) {
  auto &self = tasking::inner(this);
  assert(self.state == tasking::UNSCHEDULED);
  storerelease(&self.state, u16(tasking::SCHEDULED));
  acquire();
  if (--self.tostart == 0) {
    storerelease(&self.state, u16(tasking::RUNNING));
    self.owner->append(this);
  }
}

void task::wait(void) { tasking::inner(this).wait(false); }

void task::starts(task &dep) {
  auto &self = tasking::inner(this);
  auto &other = tasking::inner(&dep);
  assert(self.state == tasking::UNSCHEDULED && other.state == tasking::UNSCHEDULED);
  const u32 startindex = self.tasktostartnum++;
  const u32 depindex = other.depnum++;
  assert(startindex < tasking::MAXSTART && depindex < tasking::MAXDEP);
  self.taskstostart[startindex] = &dep;
  other.deps[depindex] = this;
  acquire();
  dep.acquire();
  other.tostart++;
}

void task::ends(task &dep) {
  auto &self = tasking::inner(this);
  auto &other = tasking::inner(&dep);
  assert(self.state == tasking::UNSCHEDULED && other.state < tasking::DONE);
  const u32 endindex = self.tasktoendnum++;
  const u32 depindex = other.depnum++;
  assert(endindex < tasking::MAXEND && depindex < tasking::MAXDEP);
  self.taskstoend[endindex] = &dep;
  other.deps[depindex] = this;
  acquire();
  dep.acquire();
  other.toend++;
}
} /* namespace q */

