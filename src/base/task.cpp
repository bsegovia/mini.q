/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - task.hpp -> implements multi-threaded tasking system
 -------------------------------------------------------------------------*/
#include "base/task.hpp"
#include "base/vector.hpp"
#include "base/math.hpp"
#include "base/intrusive_list.hpp"
#include <SDL_thread.h>

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

// a set of threads subscribes this queue and share the work in the list of the
// tasks. threads terminate when "terminatethreads" become true
struct queue {
  queue(u32 threadnum);
  ~queue(void);
  void append(task*);
  void remove(task*);
  void terminate(task*);
  static int threadfunc(void*);
  SDL_cond *cond;
  SDL_mutex *mutex;
  vector<SDL_Thread*> threads;
  intrusive_list<task> readylist;
  volatile bool terminatethreads;
};

void queue::append(task *job) {
  assert(job->owner == this && job->tostart == 0);
  SDL_LockMutex(job->owner->mutex);
    if (job->elemnum > 0) {
      job->acquire();
      if (job->policy & task::HI_PRIO)
        job->owner->readylist.push_front(job);
      else
        job->owner->readylist.push_back(job);
    }
  SDL_CondBroadcast(cond);
  SDL_UnlockMutex(job->owner->mutex);
}

void queue::remove(task *job) {
  assert(job->owner == this);
  SDL_LockMutex(job->owner->mutex);
  if (!job->in_list()) {
    SDL_UnlockMutex(job->owner->mutex);
    return;
  }
  job->owner->readylist.erase(job);
  SDL_UnlockMutex(job->owner->mutex);
  job->release();
}

void queue::terminate(task *job) {
  assert(job->owner == this && job->toend == 0);
  storerelease(&job->state, u16(DONE));

  // go over all tasks that depend on us
  loopi(job->tasktostartnum) {
    if (--job->taskstostart[i]->tostart == 0)
      append(job->taskstostart[i]);
    job->taskstostart[i]->release();
  }
  loopi(job->tasktoendnum) {
    if (--job->taskstoend[i]->toend == 0)
      terminate(job->taskstoend[i]);
    job->taskstoend[i]->release();
  }

  // if no more waiters, we can safely free all dependencies since we are done
  if (job->waiternum == 0)
    loopi(job->depnum) job->deps[i]->release();
}

int queue::threadfunc(void *data) {
#if defined(__X86__) || defined(__X86_64__)
  // flush to zero and no denormals
  _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));
#endif
  auto q = (queue*) data;
  for (;;) {
    SDL_LockMutex(q->mutex);
    while (q->readylist.empty() && !q->terminatethreads)
      SDL_CondWait(q->cond, q->mutex);
    if (q->terminatethreads) {
      SDL_UnlockMutex(q->mutex);
      break;
    }
    auto job = q->readylist.front();

    // if unfair, we run all elements until there is nothing else to do in this
    // job. we do not care if a hi-prio job arrives while we run
    job->acquire();
    if (job->policy & task::UNFAIR) {
      SDL_UnlockMutex(q->mutex);
      for (;;) {
        const auto elt = --job->elemnum;
        if (elt >= 0) {
          job->run(elt);
          if (--job->toend == 0) q->terminate(job);
        }
        if (elt == 0) q->remove(job);
        if (elt <= 0) break;
      }
    }
    // if fair, we run once and go back to the queue to possibly run something
    // with hi-prio that just arrived
    else {
      const auto elt = --job->elemnum;
      if (elt == 0) q->remove(job);
      SDL_UnlockMutex(q->mutex);
      if (elt >= 0) {
        job->run(elt);
        if (--job->toend == 0) q->terminate(job);
      }
    }
    job->release();
  }

  return 0;
}

queue::queue(u32 threadnum) : terminatethreads(false) {
  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
  loopi(s32(threadnum)) threads.push_back(SDL_CreateThread(threadfunc, "worker thread", this));
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
  tasking::queues.resize(n);
  loopi(s32(n)) tasking::queues[i] = NEW(tasking::queue, queueinfo[i]);
}

void task::finish(void) {
  loopv(tasking::queues) DEL(tasking::queues[i]);
  tasking::queues = vector<tasking::queue*>();
}

task::task(const char *name, u32 n, u32 waiternum, u32 queue, u16 policy) :
  owner(tasking::queues[queue]), name(name), elemnum(n), tostart(1), toend(n),
  depnum(0), waiternum(waiternum), tasktostartnum(0), tasktoendnum(0),
  policy(policy), state(tasking::UNSCHEDULED)
{
  assert(n > 0 && "cannot create a task with no work to do");
}

void task::run(u32 elt) {}

void task::scheduled(void) {
  assert(state == tasking::UNSCHEDULED);
  storerelease(&state, u16(tasking::SCHEDULED));
  if (--tostart == 0) {
    storerelease(&state, u16(tasking::RUNNING));
    owner->append(this);
  }
}

void task::starts(task &other) {
  assert(state == tasking::UNSCHEDULED && other.state == tasking::UNSCHEDULED);
  const u32 startindex = tasktostartnum++;
  const u32 depindex = other.depnum++;
  assert(startindex < tasking::MAXSTART && depindex < tasking::MAXDEP);
  taskstostart[startindex] = &other;
  other.deps[depindex] = this;
  acquire();
  other.acquire();
  other.tostart++;
}

void task::ends(task &other) {
  assert(state == tasking::UNSCHEDULED && other.state < tasking::DONE);
  const u32 endindex = tasktoendnum++;
  const u32 depindex = other.depnum++;
  assert(endindex < tasking::MAXEND && depindex < tasking::MAXDEP);
  taskstoend[endindex] = &other;
  other.deps[depindex] = this;
  acquire();
  other.acquire();
  other.toend++;
}

void task::wait(bool recursivewait) {
  assert((recursivewait||state >= tasking::SCHEDULED) &&
         (recursivewait||waiternum > 0));
  acquire();

  // execute all starting dependencies
  while (tostart) loopi(depnum)
    if (deps[i]->toend) deps[i]->wait(true);

  // execute the run function
  for (;;) {
    const auto elt = --elemnum;
    if (elt == 0) owner->remove(this);
    if (elt >= 0) {
      run(elt);
      if (--toend == 0) owner->terminate(this);
    }
    if (elt <= 0) break;
  }

  // execute all ending dependencies. n.a.u.g.h.t.y -> busy waiting here
#if defined(__SSE__)
  int pausenum = 1;
#endif /* __SSE__ */

  while (toend) {
    loopi(depnum)
      if (deps[i]->toend) deps[i]->wait(true);
#if defined(__SSE__)
      else
        loopj(pausenum) _mm_pause();
      pausenum = min(pausenum*2, 64);
#endif /* __SSE__ */
  }

  // finished and no more waiters, we can safely release the dependency array
  if (!recursivewait && --waiternum == 0)
    loopi(depnum) deps[i]->release();
  release();
}

} /* namespace q */

