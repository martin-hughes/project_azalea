/// @file
/// @brief Implementation of mutexes in Azalea kernel.

//#define ENABLE_TRACING

#include <mutex>

#include "tracing.h"
#include "k_assert.h"
#include "panic.h"
#include "types/mutex.h"
#include "types/list.h"
#include "processor.h"
#include "timing.h"

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
  ASSERT(owner_thread == task_get_cur_thread());
  unlock_ignore_owner();
  KL_TRC_EXIT;
}

/// @brief Unlock the mutex, ignoring whether or not this thread is currently the owner.
///
/// This should be used sparingly. Consider using semaphores instead.
void ipc::base_mutex::unlock_ignore_owner()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Releasing mutex ", this, " from thread ", task_get_cur_thread(), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Owner thread: ", owner_thread, "\n");

  std::lock_guard<ipc::spinlock> guard(access_lock);

  klib_list_item<std::shared_ptr<task_thread>> *next_owner;

  ASSERT(lock_count > 0);

  lock_count--;

  if (lock_count == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This thread releases mutex\n");
    next_owner = waiting_threads_list.head;

    if (next_owner == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No next owner for the mutex, release\n");
      owner_thread = nullptr;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Getting next owner from the head of list\n");
      KL_TRC_TRACE(TRC_LVL::EXTRA, "Next owner is", next_owner->item.get(), "\n");
      owner_thread = next_owner->item.get();
      lock_count = 1;
      klib_list_remove(next_owner);
      next_owner->item->start_thread();
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Still keeping mutex, count reduced\n");
  }

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

  access_lock.lock();

  if (lock_count && (owner_thread == task_get_cur_thread()))
  {
    ASSERT(recursive);
    lock_count++;
  }
  else if (lock_count == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex unlocked, so acquire now.\n");
    lock_count = 1;
    owner_thread = task_get_cur_thread();
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Locked in: ", task_get_cur_thread(), " (", owner_thread, ")\n");
  }
  else if (wait_in_us == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked, but no timeout, so return now.\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked, timed or indefinite wait.\n");

    // Wait for the mutex to become free. Add this thread to the list of waiting threads, then suspend this thread.
    task_thread *this_thread = task_get_cur_thread();
    ASSERT(this_thread != nullptr);
    ASSERT(!klib_list_item_is_in_any_list(this_thread->synch_list_item));
    ASSERT(this_thread->synch_list_item->item.get() == this_thread);

    ASSERT(owner_thread != nullptr);

    klib_list_add_tail(&waiting_threads_list, this_thread->synch_list_item);

    // To avoid marking this thread as not being scheduled before freeing the lock - which would deadlock anyone else
    // trying to use this mutex, stop scheduling for the time being.
    task_continue_this_thread();
    this_thread->stop_thread();

    // If there is a period to wait then specify it to the scheduler now. The scheduler won't react until after
    // scheduling is resumed.
    if (wait_in_us != ipc::MAX_WAIT)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set thread wakeup time\n");
      this_thread->wake_thread_after = time_get_system_timer_count(true) + (1000 * wait_in_us);
    }

    // Freeing the lock means that we could immediately become the owner thread. That's OK, we'll check once we come
    // back to this code after yielding.
    access_lock.unlock();

    // Don't yield without resuming normal scheduling, otherwise we'll come straight back here without acquiring the
    // mutex. Once task_yield is called, the scheduler won't resume this process because it has been removed from the
    // running list by task_stop_thread.
    task_resume_scheduling();
    task_yield();

    // We've been scheduled again! We should now own the mutex.
    access_lock.lock();
    ASSERT(lock_count > 0);
    ASSERT((!(wait_in_us == ipc::MAX_WAIT)) || (owner_thread == this_thread));
    if (owner_thread == this_thread)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex: ", this, " in thread ", task_get_cur_thread(), "\n");
      result = true;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed to acquire mutex before timeout\n");
      result = false;
      klib_list_remove(this_thread->synch_list_item);
    }
  }

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex lock count: ", lock_count, " Owner: ", owner_thread, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "This thread: ", task_get_cur_thread(), "\n");
    ASSERT((lock_count > 0) && (owner_thread == task_get_cur_thread()));
  }

  access_lock.unlock();

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

// Put these functions here to allow them to be replaced by test versions in the unit test code.
ipc::mutex::mutex() noexcept : base_mutex{} { }
ipc::mutex::mutex(bool recursive) noexcept : base_mutex{recursive} { }
ipc::mutex::~mutex() { }
