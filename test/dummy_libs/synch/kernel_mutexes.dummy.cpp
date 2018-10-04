// KLIB mutex dummy implementation for test scripts.
// The mutex implementation in the main code relies upon the task scheduling system, which can't easily be emulated in
// the test code. As such, create a dummy implementation here.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include <thread>
#include <mutex>
#include <memory>
#include <map>

std::map<klib_mutex *, std::unique_ptr<std::mutex>> mutex_map;

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

  mutex_map[&mutex] = std::make_unique<std::mutex>();

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

  bool locked_ok;
  SYNC_ACQ_RESULT our_result;

  // This isn't quite right - we don't support timed waits here.
  if (max_wait != 0)
  {
    mutex_map[&mutex]->lock();
    locked_ok = true;
  }
  else
  {
    locked_ok = mutex_map[&mutex]->try_lock();
  }

  if (locked_ok)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex\n");
    our_result = SYNC_ACQ_ACQUIRED;
    mutex.mutex_locked = true;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Already owned mutex\n");
    our_result = SYNC_ACQ_TIMEOUT;
    mutex.mutex_locked = true;
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
  mutex_map[&mutex]->unlock();

  KL_TRC_EXIT;
}

void test_only_free_mutex(klib_mutex &mutex)
{
  mutex_map.erase(&mutex);
}