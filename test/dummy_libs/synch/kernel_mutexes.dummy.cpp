// KLIB mutex dummy implementation for test scripts.
// The mutex implementation in the main code relies upon the task scheduling system, which can't easily be emulated in
// the test code. As such, create a dummy implementation here.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "pthread.h"

#include <map>

std::map<klib_mutex *, pthread_mutex_t> mutex_map;

// Initialize a mutex object. The owner of the mutex object is responsible for managing the memory associated with it.
void klib_synch_mutex_init(klib_mutex &mutex)
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_init(mutex.access_lock);
  klib_synch_spinlock_lock(mutex.access_lock);

  mutex.mutex_locked = false;
  mutex.owner_thread = nullptr;

  klib_list_initialize(&mutex.waiting_threads_list);
  klib_synch_spinlock_unlock(mutex.access_lock);

  mutex_map[&mutex] = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

  KL_TRC_EXIT;
}

// Acquire the mutex for the currently running thread. It is permissible for a thread to call this function when it
// already owns the mutex - nothing happens. The maximum time to wait is max_wait milliseconds. If max_wait is set to
// MUTEX_MAX_WAIT then the caller waits indefinitely. Threads acquire the mutex in order that they call this function.
//
// The return values should be self-explanatory.
SYNC_ACQ_RESULT klib_synch_mutex_acquire(klib_mutex &mutex, uint64_t max_wait)
{
  KL_TRC_ENTRY;

  int pthread_lock_result;
  SYNC_ACQ_RESULT our_result;

  // This isn't quite right - we don't support timed waits here.
  if (max_wait != 0)
  {
    pthread_lock_result = pthread_mutex_lock(&mutex_map[&mutex]);
  }
  else
  {
    pthread_lock_result = pthread_mutex_trylock(&mutex_map[&mutex]);
  }

  switch (pthread_lock_result)
  {
    case 0:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex\n");
      our_result = SYNC_ACQ_ACQUIRED;
      mutex.mutex_locked = true;
      break;

    case EDEADLK:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Already owned mutex\n");
      our_result = SYNC_ACQ_ALREADY_OWNED;
      mutex.mutex_locked = true;
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Error occurred or timeout\n");
      our_result = SYNC_ACQ_TIMEOUT;
      mutex.mutex_locked = false;
      break;
  }

  KL_TRC_EXIT;

  return our_result;
}

// Release the mutex. If a thread is waiting for it, it will be permitted to run.
void klib_synch_mutex_release(klib_mutex &mutex, const bool disregard_owner)
{
  KL_TRC_ENTRY;

  ASSERT(mutex.mutex_locked);
  mutex.mutex_locked = false;
  pthread_mutex_unlock(&mutex_map[&mutex]);

  KL_TRC_EXIT;
}
