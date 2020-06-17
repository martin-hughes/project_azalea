// Mutex dummy implementation for test scripts.
// The mutex implementation in the main code relies upon the task scheduling system, which can't easily be emulated in
// the test code. As such, create a dummy implementation here.

// Known defects:
// - This doesn't bother to check that non-recursive mutexes aren't recursed.

//#define ENABLE_TRACING

#include <thread>
#include <mutex>
#include <memory>
#include <map>

#include "tracing.h"
#include "types/mutex.h"
#include "map_helpers.h"
#include "processor.h"

std::map<ipc::base_mutex *, std::unique_ptr<std::recursive_timed_mutex>> mutex_map;

/// @brief Standard destructor.
ipc::base_mutex::~base_mutex()
{
  KL_TRC_ENTRY;

  // Don't attempt to tidy up other threads or anything like that because this mutex won't exist by the time they start
  // running, so they will access invalid memory, and probably crash anyway.
  ASSERT(lock_count == 0);

  KL_TRC_EXIT;
}

/// @brief Lock the mutex.
///
/// Wait for ever if necessary.
void ipc::base_mutex::lock()
{
  ASSERT(timed_lock(ipc::MAX_WAIT));
}

/// @brief Try to lock the mutex if it is uncontested.
///
/// @return Whether or not the mutex was locked.
bool ipc::base_mutex::try_lock()
{
  return timed_lock(0);
}

/// @brief Unlock the mutex.
void ipc::base_mutex::unlock()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Releasing mutex ", this, " from thread ", task_get_cur_thread(), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Owner thread: ", owner_thread, "\n");

  mutex_map[this]->unlock();

  KL_TRC_EXIT;
}

/// @brief Attempt to lock the mutex, but with a timeout.
///
/// @param wait_in_us The maximum number of microseconds to wait while trying to lock the mutex. If this is
///                   ipc::MAX_WAIT, then wait indefinitely.
///
/// @return True if the mutex was locked, false otherwise.
bool ipc::base_mutex::timed_lock(uint64_t wait_in_us)
{
  bool result{true};

  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Acquiring mutex ", this, " in thread ", task_get_cur_thread(), "\n");

  if (wait_in_us != ipc::MAX_WAIT)
  {
    using mu_s = std::chrono::microseconds;
    result = mutex_map[this]->try_lock_for(mu_s(wait_in_us));
  }
  else
  {
    mutex_map[this]->lock();
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Is the calling thread the owner of this mutex, if it is locked?
///
/// @return If the mutex is locked, true if this thread is the current owner. False otherwise.
bool ipc::base_mutex::am_owner()
{
  bool result = (owner_thread == task_get_cur_thread());
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

ipc::mutex::mutex() noexcept : mutex{false} { }

ipc::mutex::mutex(bool recursive) noexcept : base_mutex{recursive}
{
  ipc::base_mutex *t = this;
  ASSERT(!map_contains(mutex_map, t));
  mutex_map[this] = std::make_unique<std::recursive_timed_mutex>();
}

ipc::mutex::~mutex()
{
  ipc::base_mutex *t = this;
  ASSERT(map_contains(mutex_map, t));
  mutex_map.erase(this);
}
