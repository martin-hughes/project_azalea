// KLIB Semaphores implementation

#include "klib/klib.h"

// Initialize a semaphore object. The owner of the semaphore object is responsible for managing the memory associated
// with it.
void klib_synch_semaphore_init(klib_semaphore &semaphore, unsigned long max_users, unsigned long start_users)
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

// Acquire the semaphore for the currently running thread. It is not permissible for a thread to call this function
// when it already owns the semaphore - the thread may become permanently unscheduled, and hence blocked. The maximum
// time to wait is max_wait milliseconds. If max_wait is set to SEMAPHORE_MAX_WAIT then the caller waits indefinitely.
// Threads acquire the semaphore in order that they call this function.
//
// The return values should be self-explanatory.
//
// TODO: Add support for max_wait.
SYNC_ACQ_RESULT klib_synch_semaphore_wait(klib_semaphore &semaphore, unsigned long max_wait)
{
  KL_TRC_ENTRY;

  SYNC_ACQ_RESULT res = SYNC_ACQ_TIMEOUT;

  ASSERT ((max_wait == 0) || (max_wait == SEMAPHORE_MAX_WAIT));

  klib_synch_spinlock_lock(semaphore.access_lock);

  if (semaphore.cur_user_count < semaphore.max_users)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Immediately acquired\n"));
    semaphore.cur_user_count++;
    res = SYNC_ACQ_ACQUIRED;
  }
  else if (max_wait == 0)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "No spare slots and immediate fallback\n"));
    res = SYNC_ACQ_TIMEOUT;
  }
  else
  {
    ASSERT(max_wait == SEMAPHORE_MAX_WAIT);

    KL_TRC_TRACE((TRC_LVL_FLOW, "Semaphore full, indefinite wait.\n"));

    // Wait for the semaphore to become free. Add this thread to the list of waiting threads, then suspend this thread.
    task_thread *this_thread = task_get_cur_thread();
    klib_list_item *item = new klib_list_item;

    ASSERT(semaphore.cur_user_count == semaphore.max_users);

    klib_list_item_initialize(item);
    item->item = (void *)this_thread;

    klib_list_add_tail(&semaphore.waiting_threads_list, item);

    // To avoid marking this thread as not being scheduled before freeing the lock - which would deadlock anyone else
    // trying to use this semaphore - stop scheduling for the time being.
    task_continue_this_thread();
    task_stop_thread(this_thread);

    // Freeing the lock means that we could immediately become the owner thread. That's OK, we'll check once we come
    // back to this code after yielding.
    klib_synch_spinlock_unlock(semaphore.access_lock);

    // Don't yield without resuming normal scheduling, otherwise we'll come straight back here without acquiring the
    // semaphore. Once task_yield is called, the scheduler won't resume this process because it has been removed from
    // the running list by task_stop_thread.
    task_resume_scheduling();
    task_yield();

    // We've been scheduled again! We've been signalled past the semaphore.
    klib_synch_spinlock_lock(semaphore.access_lock);

    res = SYNC_ACQ_ACQUIRED;
  }

  klib_synch_spinlock_unlock(semaphore.access_lock);

  KL_TRC_EXIT;

  return res;
}

// Release the semaphore. If a thread is waiting for it, it will be permitted to run.
void klib_synch_semaphore_clear(klib_semaphore &semaphore)
{
  KL_TRC_ENTRY;

  klib_list_item *next_owner;

  klib_synch_spinlock_lock(semaphore.access_lock);

  next_owner = semaphore.waiting_threads_list.head;
  if (next_owner == NULL)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "No next user for the semaphore, release\n"));
    ASSERT(semaphore.cur_user_count > 0);
    semaphore.cur_user_count--;
  }
  else
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "Getting next user from the head of list\n"));
    KL_TRC_DATA("Next user is", (unsigned long)next_owner->item);
    ASSERT(semaphore.cur_user_count == semaphore.max_users);
    klib_list_remove(next_owner);
    task_start_thread((task_thread *)next_owner->item);
    delete next_owner;
    next_owner = (klib_list_item *)NULL;
  }

  klib_synch_spinlock_unlock(semaphore.access_lock);


  KL_TRC_EXIT;
}
