/// @file
/// @brief Implementation of common synchronization objects that threads may choose to wait on.
//
// Known defects:
// - It isn't possible to report whether "wait_for_signal()" was successful or not...
// - Signalling a semaphore beyond empty will assert.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/synch_objects.h"
#include "processor/timing/timing.h"

WaitObject::WaitObject()
{
  KL_TRC_ENTRY;

  klib_list_initialize(&this->_waiting_threads);
  klib_synch_spinlock_init(this->_list_lock);

  KL_TRC_EXIT;
}

WaitObject::~WaitObject()
{
  KL_TRC_ENTRY;

  // To ensure that nothing gets deadlocked, signal all waiting threads now.
  this->trigger_all_threads();

  KL_TRC_EXIT;
}

/// @brief Cause this thread to wait until the WaitObject is triggered, at which point it will resume.
///
/// @param max_wait The approximate maximum time to wait for the object to be signalled, in microseconds.
void WaitObject::wait_for_signal(uint64_t max_wait)
{
  KL_TRC_ENTRY;

  task_thread *cur_thread = task_get_cur_thread();
  ASSERT(cur_thread);
  ASSERT(!cur_thread->is_worker_thread);
  klib_list_item<task_thread *> *list_item = new klib_list_item<task_thread *>;
  klib_list_item_initialize(list_item);
  list_item->item = cur_thread;

  klib_synch_spinlock_lock(this->_list_lock);
  task_continue_this_thread();

  cur_thread->stop_thread();
  klib_list_add_tail(&this->_waiting_threads, list_item);
  klib_synch_spinlock_unlock(this->_list_lock);

  if (max_wait != WaitObject::MAX_WAIT)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Set maximum waiting time");
    cur_thread->wake_thread_after = time_get_system_timer_count() + (max_wait * 1000);
  }

  task_resume_scheduling();

  // Having added ourselves to the list we should not pass through task_yield() until the thread is re-awakened below.
  // It is possible that the thread was signalled between the list being unlocked above and here, in which case it is
  // reasonable to just carry on.
  task_yield();

  KL_TRC_EXIT;
}

/// @brief Cause the parameter thread to resume immediately.
///
/// If the thread is already resumed or is not waiting on this object then this call has no effect.
///
/// @param thread The thread to resume
///
/// There is no indication to the thread that it has resumed prematurely.
void WaitObject::cancel_waiting_thread(task_thread *thread)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for thread ", thread, "\n");

  klib_list_item<task_thread *> *list_item;
  klib_synch_spinlock_lock(this->_list_lock);

  list_item = this->_waiting_threads.head;

  while((list_item != nullptr) && (list_item->item != thread))
  {
    list_item = list_item->next;
  }

  if ((list_item != nullptr) && (list_item->item == thread))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Removing thread and resuming it\n");
    klib_list_remove(list_item);

    delete list_item;
    list_item = nullptr;

    thread->start_thread();
  }

  klib_synch_spinlock_unlock(this->_list_lock);

  KL_TRC_EXIT;
}

/// @brief Return a count of how many threads are waiting on this WaitObject.
///
/// @return The number of waiting threads.
uint64_t WaitObject::threads_waiting()
{
  KL_TRC_ENTRY;

  uint64_t result;
  result = klib_list_get_length(&this->_waiting_threads);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Permit the next waiting thread to proceed.
///
/// In the default WaitObject implementation threads are triggered one-by-one, in the same order as which they waited
/// on this WaitObject, but derived classes can choose any implementation. If no threads are waiting, nothing happens.
///
/// @param should_lock Set this to false if this function is being called from a function that already holds
///                    _list_lock. Otherwise, leave as true.
void WaitObject::trigger_next_thread(const bool should_lock)
{
  KL_TRC_ENTRY;

  if (should_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "locking\n");
    klib_synch_spinlock_lock(this->_list_lock);
  }

  klib_list_item<task_thread *> *list_item = this->_waiting_threads.head;
  if (list_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Starting thread ", list_item->item, "\n");
    klib_list_remove (list_item);
    list_item->item->start_thread();
    delete list_item;
    list_item = nullptr;
  }

  if (should_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unlocking\n");
    klib_synch_spinlock_unlock(this->_list_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Trigger all threads waiting for this object to continue.
///
/// This may not be a valid operation for all child types.
void WaitObject::trigger_all_threads()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Triggering from object: ", this, "\n");

  klib_synch_spinlock_lock(this->_list_lock);
  while(klib_list_get_length(&this->_waiting_threads) != 0)
  {
    trigger_next_thread(false);
  }
  klib_synch_spinlock_unlock(this->_list_lock);

  KL_TRC_EXIT;
}

WaitForFirstTriggerObject::WaitForFirstTriggerObject() :
  WaitObject{ },
  already_triggered{false}
{

}

WaitForFirstTriggerObject::~WaitForFirstTriggerObject()
{
  KL_TRC_ENTRY;
  trigger_all_threads();
  KL_TRC_EXIT;
}

void WaitForFirstTriggerObject::wait_for_signal(uint64_t max_wait)
{
  KL_TRC_ENTRY;

  task_thread *cur_thread = task_get_cur_thread();
  ASSERT(cur_thread);
  ASSERT(!cur_thread->is_worker_thread);
  klib_list_item<task_thread *> *list_item = new klib_list_item<task_thread *>;
  klib_list_item_initialize(list_item);
  list_item->item = cur_thread;

  klib_synch_spinlock_lock(this->_list_lock);

  if (!this->already_triggered)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not yet triggered, wait. (", this, ")\n");
    task_continue_this_thread();

    cur_thread->stop_thread();
    klib_list_add_tail(&this->_waiting_threads, list_item);
    klib_synch_spinlock_unlock(this->_list_lock);

    if (max_wait != WaitObject::MAX_WAIT)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set maximum waiting time");
      cur_thread->wake_thread_after = time_get_system_timer_count() + (max_wait * 1000);
    }

    task_resume_scheduling();

    // Having added ourselves to the list we should not pass through task_yield() until the thread is re-awakened below.
    // It is possible that the thread was signalled between the list being unlocked above and here, in which case it is
    // reasonable to just carry on.
    task_yield();
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Triggered, continue. (", this, ")\n");
    klib_synch_spinlock_unlock(this->_list_lock);
  }

  KL_TRC_EXIT;
}

void WaitForFirstTriggerObject::trigger_next_thread(const bool should_lock)
{
  KL_TRC_ENTRY;

  if (should_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "locking\n");
    klib_synch_spinlock_lock(this->_list_lock);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Setting triggered to true.\n");
  already_triggered = true;

  klib_list_item<task_thread *> *list_item = this->_waiting_threads.head;
  if (list_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Starting thread ", list_item->item, "\n");
    klib_list_remove (list_item);
    list_item->item->start_thread();
    delete list_item;
    list_item = nullptr;
  }

  if (should_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unlocking\n");
    klib_synch_spinlock_unlock(this->_list_lock);
  }

  KL_TRC_EXIT;
}

void WaitForFirstTriggerObject::trigger_all_threads()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Triggering from object: ", this, "\n");

  klib_synch_spinlock_lock(this->_list_lock);

  this->already_triggered = true;
  while(klib_list_get_length(&this->_waiting_threads) != 0)
  {
    trigger_next_thread(false);
  }

  klib_synch_spinlock_unlock(this->_list_lock);

  KL_TRC_EXIT;
}

/// @brief Create a mutex object that can be exposed by the system call API to user processes
///
syscall_mutex_obj::syscall_mutex_obj()
{
  KL_TRC_ENTRY;

  klib_synch_mutex_init(this->base_mutex);

  KL_TRC_EXIT;
}

/// @brief Destroy this object.
///
syscall_mutex_obj::~syscall_mutex_obj()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}

void syscall_mutex_obj::wait_for_signal(uint64_t max_wait)
{
  KL_TRC_ENTRY;

  klib_synch_mutex_acquire(base_mutex, max_wait);

  KL_TRC_EXIT;
}

/// @brief Release the mutex.
///
/// @return True if this thread previously owned the mutex and has now released it. False otherwise.
bool syscall_mutex_obj::release()
{
  bool result{true};

  KL_TRC_ENTRY;

  // It is possible for mutex_locked and owner_thread to change part way through this if statement - but not if both
  // parts are true, because otherwise this thread would be executing in two places at once, which is a contradiction.
  if (base_mutex.mutex_locked && (base_mutex.owner_thread == task_get_cur_thread()))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Release the mutex!\n");
    klib_synch_mutex_release(base_mutex, false);
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "This thread doesn't own the mutex anyway...\n");
    result = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

void syscall_mutex_obj::trigger_next_thread(const bool should_lock)
{
  panic("Not valid for mutexes");
}

void syscall_mutex_obj::trigger_all_threads()
{
  panic("Not valid for mutexes");
}

/// @brief Create a semaphore object that can be exposed by the system call API to user mode processes.
///
/// @param max_users The maximum number of threads that can hold the semaphore at once.
///
/// @param start_users How many users should the semaphore consider itself to be held by at the start?
syscall_semaphore_obj::syscall_semaphore_obj(uint64_t max_users, uint64_t start_users)
{
  KL_TRC_ENTRY;

  klib_synch_semaphore_init(base_semaphore, max_users, start_users);

  KL_TRC_EXIT;
}

/// @brief Destroy the semaphore.
///
syscall_semaphore_obj::~syscall_semaphore_obj()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
}

void syscall_semaphore_obj::wait_for_signal(uint64_t max_wait)
{
  KL_TRC_ENTRY;

  klib_synch_semaphore_wait(base_semaphore, max_wait);

  KL_TRC_EXIT;
}

/// @brief Signal this semaphore.
bool syscall_semaphore_obj::signal()
{
  KL_TRC_ENTRY;

  klib_synch_semaphore_clear(base_semaphore);

  KL_TRC_EXIT;

  return true;
}

void syscall_semaphore_obj::trigger_next_thread(const bool should_lock)
{
  panic("Not valid for semaphores");
}

void syscall_semaphore_obj::trigger_all_threads()
{
  panic("Not valid for semaphores");
}
