/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - task.hpp -> implements multi-threaded tasking system
 -------------------------------------------------------------------------*/
#include "base/task.hpp"
#include "base/vector.hpp"
#include "base/math.hpp"
#include "base/intrusive_list.hpp"
#include "base/sys.hpp"
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
    if (--job->tasktostart[i]->tostart == 0)
      append(job->tasktostart[i]);
    job->tasktostart[i] = NULL;
  }
  loopi(job->tasktoendnum) {
    if (--job->tasktoend[i]->toend == 0)
      terminate(job->tasktoend[i]);
    job->tasktoend[i] = NULL;
  }
  job->tasktoendnum = 0;
  job->tasktostartnum = 0;
}

struct thread_data {
  queue *q;
  int index;
};

int queue::threadfunc(void *data) {
#if defined(__X86__) || defined(__X86_64__)
  // flush to zero and no denormals
  _mm_setcsr(_mm_getcsr() | (1<<15) | (1<<6));
#endif
  const auto td = (thread_data*) data;
  const auto q = (queue*) td->q;
  // sys::set_affinity(td->index+1);
  DEL(td);
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
  loopi(threadnum) {
    const auto td = NEWE(thread_data);
    td->index = i;
    td->q = this;
    threads.push_back(SDL_CreateThread(threadfunc, "worker thread", td));
  }
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

task::task(const char *name, u32 n, u32 waiternum, u32 queue, u16 policy) :
  owner(tasking::queues[queue]), name(name), elemnum(n), tostart(1), toend(n),
  depnum(0), waiternum(waiternum), tasktostartnum(0), tasktoendnum(0),
  policy(policy), state(tasking::UNSCHEDULED)
{
  assert(n > 0 && "cannot create a task with no work to do");
}
task::~task() {}

void task::start(const u32 *queueinfo, u32 n) {
  // sys::set_affinity(0);
  tasking::queues.resize(n);
  loopi(n) tasking::queues[i] = NEW(tasking::queue, queueinfo[i]);
}

void task::finish(void) {
  loopv(tasking::queues) DEL(tasking::queues[i]);
  tasking::queues = vector<tasking::queue*>();
}

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
  other.tostart++;

  // compare-and-swap loop to safely both number of tasks to start and the task
  // itself. Be careful with race condition, we first need to put the task and
  // then to update the counter
  auto old = 0;
  do {
    old = tasktostartnum;
    assert(old < task::MAXSTART);
    tasktostart[old] = &other;
  } while (cmpxchg(tasktostartnum, old+1, old));

  // same strategy for the dependency array
  do {
    old = other.depnum;
    assert(old < task::MAXDEP);
    other.deps[old] = this;
  } while (cmpxchg(other.depnum, old+1, old));
}

void task::ends(task &other) {
  assert(state == tasking::UNSCHEDULED && other.state < tasking::DONE);
  other.toend++;

  // compare-and-swap loop as above
  auto old = 0;
  do {
    old = tasktoendnum;
    assert(old < task::MAXEND);
    tasktoend[old] = &other;
  } while (old != cmpxchg(tasktoendnum, old+1, old));

  // same strategy for the dependency array
  do {
    old = other.depnum;
    assert(old < task::MAXDEP);
    other.deps[old] = this;
  } while (old != cmpxchg(other.depnum, old+1, old));
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
  release();
}

} /* namespace q */

