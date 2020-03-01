/// @file
/// @brief KLIB Semaphores implementation

// Known defects:
// - There is no checking that a thread releasing the semaphore previously held it.

// #define ENABLE_TRACING

#include "processor/processor.h"
#include "processor/timing/timing.h"
#include "klib/klib.h"

/// @brief Initialize a semaphore object.
///
/// The owner of the semaphore object is responsible for managing the memory associated with the semaphore object.
///
/// @param semaphore The semaphore to initialise.
///
/// @param max_users The maximum number of threads that can hold the semaphore at once.
///
/// @param start_users How many users should the semaphore consider itself to be held by at the start?
void klib_synch_semaphore_init(klib_semaphore &semaphore, uint64_t max_users, uint64_t start_users)
{
  KL_TRC_ENTRY;
  ASSERT (max_users != 0);

  klib_synch_spinlock_init(semaphore.access_lock);
  klib_synch_spinlock_lock(semaphore.access_lock);

  semaphore.cur_user_count = start_users;
  semaphore.max_users = max_users;

  klib_list_initialize(&semaphore.waiting_threads_list);

  klib_synch_spinlock_unlock(semaphore.access_lock);

  KL_TRC_EXIT;
}

/// @brief Acquire the semaphore for the currently running thread.
///
/// If the semaphore is not currently being held by max_users threads, acquire the semaphore now, otherwise wait for
/// space to acquire it.
///
/// It is not permissible for a thread to call this function when it already owns the semaphore - the thread may become
/// permanently unscheduled, and hence blocked. Threads acquire the semaphore in order that they call this function.
///
/// @param semaphore The semaphore to acquire.
///
/// @param max_wait The maximum time to wait in microseconds. If max_wait is set to SEMAPHORE_MAX_WAIT then the caller
///                 waits indefinitely. If set to zero, this function does not block. Timed waits other than zero or
///                 SEMAPHORE_MAX_WAIT are not currently supported.
///
/// @return SYNC_ACQ_ACQUIRED upon success. SYNC_ACQ_ALREADY_OWNED if this thread already owns the semaphore.
///         SYNC_ACQ_TIMEOUT if the semaphore could not be acquired within max_wait.
SYNC_ACQ_RESULT klib_synch_semaphore_wait(klib_semaphore &semaphore, uint64_t max_wait)
{
  KL_TRC_ENTRY;

  SYNC_ACQ_RESULT res = SYNC_ACQ_TIMEOUT;

  ASSERT ((max_wait == 0) || (max_wait == SEMAPHORE_MAX_WAIT));

  klib_synch_spinlock_lock(semaphore.access_lock);

  if (semaphore.cur_user_count < semaphore.max_users)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Immediately acquired\n");
    semaphore.cur_user_count++;
    res = SYNC_ACQ_ACQUIRED;
  }
  else if (max_wait == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No spare slots and immediate fallback\n");
    res = SYNC_ACQ_TIMEOUT;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Semaphore full, timed or indefinite wait.\n");

    // Wait for the semaphore to become free. Add this thread to the list of waiting threads, then suspend this thread.
    task_thread *this_thread = task_get_cur_thread();
    klib_list_item<std::shared_ptr<task_thread>> *item = new klib_list_item<std::shared_ptr<task_thread>>;

    ASSERT(semaphore.cur_user_count == semaphore.max_users);

    klib_list_item_initialize(item);
    item->item = this_thread->synch_list_item->item;

    klib_list_add_tail(&semaphore.waiting_threads_list, item);

    // To avoid marking this thread as not being scheduled before freeing the lock - which would deadlock anyone else
    // trying to use this semaphore - stop scheduling for the time being.
    task_continue_this_thread();
    this_thread->stop_thread();

    // If there is a period to wait then specify it to the scheduler now. The scheduler won't react until after
    // scheduling is resumed.
    if (max_wait != SEMAPHORE_MAX_WAIT)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set thread wakeup time\n");
      this_thread->wake_thread_after = time_get_system_timer_count(true) + (1000 * max_wait);
    }

    // Freeing the lock means that we could immediately become the owner thread. That's OK, we'll check once we come
    // back to this code after yielding.
    klib_synch_spinlock_unlock(semaphore.access_lock);

    // Don't yield without resuming normal scheduling, otherwise we'll come straight back here without acquiring the
    // semaphore. Once task_yield is called, the scheduler won't resume this process because it has been removed from
    // the running list by task_stop_thread.
    task_resume_scheduling();
    task_yield();

    // We've been scheduled again! Perhaps we've been signalled past the semaphore?
    klib_synch_spinlock_lock(semaphore.access_lock);

    // If this thread still appears in the waiting threads list then we've simply timed out.
    if (!klib_list_item_is_in_any_list(this_thread->synch_list_item))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successfully acquired\n");
      res = SYNC_ACQ_ACQUIRED;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Simply timed out.\n");
      res = SYNC_ACQ_TIMEOUT;

      item = semaphore.waiting_threads_list.head;
      while (item)
      {
        if (item->item == this_thread->synch_list_item->item)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found this thread in the waiting list\n");
          klib_list_remove(item);
          delete item;
          item = nullptr;
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Move to next item\n");
          item = item->next;
        }
      }
    }
  }

  klib_synch_spinlock_unlock(semaphore.access_lock);

  KL_TRC_EXIT;

  return res;
}

/// @brief Release the semaphore.
///
/// If a thread is waiting for it, it will be permitted to run.
///
/// @param semaphore The semaphore to release.
void klib_synch_semaphore_clear(klib_semaphore &semaphore)
{
  KL_TRC_ENTRY;

  klib_list_item<std::shared_ptr<task_thread>> *next_owner;

  klib_synch_spinlock_lock(semaphore.access_lock);

  next_owner = semaphore.waiting_threads_list.head;
  if (next_owner == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No next user for the semaphore, release\n");
    ASSERT(semaphore.cur_user_count > 0);
    semaphore.cur_user_count--;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Getting next user from the head of list\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Next user is", next_owner->item, "\n");
    ASSERT(semaphore.cur_user_count == semaphore.max_users);
    klib_list_remove(next_owner);
    next_owner->item->start_thread();
    delete next_owner;
    next_owner = nullptr;
  }

  klib_synch_spinlock_unlock(semaphore.access_lock);

  KL_TRC_EXIT;
}
