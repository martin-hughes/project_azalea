// KLIB mutex implementation.

//#define ENABLE_TRACING

#include "klib/klib.h"

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

  KL_TRC_EXIT;
}

// Acquire the mutex for the currently running thread. It is permissible for a thread to call this function when it
// already owns the mutex - nothing happens. The maximum time to wait is max_wait milliseconds. If max_wait is set to
// MUTEX_MAX_WAIT then the caller waits indefinitely. Threads acquire the mutex in order that they call this function.
//
// The return values should be self-explanatory.
SYNC_ACQ_RESULT klib_synch_mutex_acquire(klib_mutex &mutex, uint64_t max_wait)
{
  SYNC_ACQ_RESULT res = SYNC_ACQ_TIMEOUT;
  klib_synch_spinlock_lock(mutex.access_lock);
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Acquiring mutex ", &mutex, " in thread ", task_get_cur_thread(), "\n");

  if ((mutex.mutex_locked == true) && (mutex.owner_thread == task_get_cur_thread()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex already owned\n");
    res = SYNC_ACQ_ALREADY_OWNED;
  }
  else if (mutex.mutex_locked == false)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex unlocked, so acquire now.\n");
    mutex.mutex_locked = true;
    mutex.owner_thread = task_get_cur_thread();
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Locked in: ", task_get_cur_thread(), " (", mutex.owner_thread, ")\n");
    res = SYNC_ACQ_ACQUIRED;
  }
  else if (max_wait == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked, but no timeout, so return now.\n");
    res = SYNC_ACQ_TIMEOUT;
  }
  else if (max_wait == MUTEX_MAX_WAIT)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked, indefinite wait.\n");

    // Wait for the mutex to become free. Add this thread to the list of waiting threads, then suspend this thread.
    task_thread *this_thread = task_get_cur_thread();
    ASSERT(this_thread != nullptr);
    ASSERT(!klib_list_item_is_in_any_list(&this_thread->synch_list_item));
    ASSERT(this_thread->synch_list_item.item == this_thread);

    ASSERT(mutex.owner_thread != nullptr);

    klib_list_add_tail(&mutex.waiting_threads_list, &this_thread->synch_list_item);

    // To avoid marking this thread as not being scheduled before freeing the lock - which would deadlock anyone else
    // trying to use this mutex, stop scheduling for the time being.
    task_continue_this_thread();
    task_stop_thread(this_thread);

    // Freeing the lock means that we could immediately become the owner thread. That's OK, we'll check once we come
    // back to this code after yielding.
    klib_synch_spinlock_unlock(mutex.access_lock);

    // Don't yield without resuming normal scheduling, otherwise we'll come straight back here without acquiring the
    // mutex. Once task_yield is called, the scheduler won't resume this process because it has been removed from the
    // running list by task_stop_thread.
    task_resume_scheduling();
    task_yield();

    // We've been scheduled again! We should now own the mutex.
    klib_synch_spinlock_lock(mutex.access_lock);
    ASSERT(mutex.mutex_locked);
    ASSERT(mutex.owner_thread == this_thread);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Acquired mutex: ", &mutex, " in thread ", task_get_cur_thread(), " (", this_thread, ")\n");

    res = SYNC_ACQ_ACQUIRED;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked, defined wait.\n");
    INCOMPLETE_CODE("Mutex timed wait");
  }

  if (res == SYNC_ACQ_ACQUIRED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mutex locked? ", mutex.mutex_locked, " Owner: ", mutex.owner_thread, "\n");
    KL_TRC_TRACE(TRC_LVL::FLOW, "This thread: ", task_get_cur_thread(), "\n");
    ASSERT(mutex.mutex_locked && (mutex.owner_thread == task_get_cur_thread()));
  }
  KL_TRC_EXIT;
  klib_synch_spinlock_unlock(mutex.access_lock);

  return res;
}

// Release the mutex. If a thread is waiting for it, it will be permitted to run.
void klib_synch_mutex_release(klib_mutex &mutex, const bool disregard_owner)
{
  klib_synch_spinlock_lock(mutex.access_lock);
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Releasing mutex ", &mutex, " from thread ", task_get_cur_thread(), "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Owner thread: ", mutex.owner_thread, "\n");

  klib_list_item<task_thread *> *next_owner;

  ASSERT(mutex.mutex_locked);
  ASSERT((disregard_owner) || (mutex.owner_thread == task_get_cur_thread()));

  next_owner = mutex.waiting_threads_list.head;
  if (next_owner == nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No next owner for the mutex, release\n");
    mutex.mutex_locked = false;
    mutex.owner_thread = nullptr;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Getting next owner from the head of list\n");
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Next owner is", next_owner->item, "\n");
    mutex.owner_thread = next_owner->item;
    klib_list_remove(next_owner);
    task_start_thread(next_owner->item);
  }

  KL_TRC_EXIT;
  klib_synch_spinlock_unlock(mutex.access_lock);
}
