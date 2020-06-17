/// @file
/// @brief Basic implementation of event-like synch objects.
///
/// Other objects may choose to extend events.
//
// Known defects:
// - It isn't possible to report whether "wait_for_signal()" was successful or not...
// - Signalling a semaphore beyond empty will assert.

//#define ENABLE_TRACING

#include "tracing.h"
#include "k_assert.h"
#include "panic.h"
#include "types/event.h"
#include "types/spinlock.h"
#include "processor.h"
#include "timing.h"

/// @brief Construct a non-resetting event.
ipc::event::event() : event{false} { }

/// @brief Construct a basic event.
///
/// @param auto_reset After the event is signalled, should it remain signalled (false) or reset itself (true)
ipc::event::event(bool auto_reset) : event{auto_reset, internal_lock} { }

/// @brief Construct an event using a different internal lock.
///
/// This allows child classes to more easily synchronize their internal operations with this class.
///
/// @param auto_reset After the event is signalled, should it remain signalled (false) or reset itself (true)
///
/// @param lock_override The lock used by the child class.
ipc::event::event(bool auto_reset, ipc::raw_spinlock &lock_override) : _list_lock{lock_override}, auto_reset{auto_reset}
{
  KL_TRC_ENTRY;

  klib_list_initialize(&this->_waiting_threads);
  ipc_raw_spinlock_init(this->_list_lock);

  KL_TRC_EXIT;
}

/// @brief Standard destructor.
ipc::event::~event()
{
  ASSERT(klib_list_is_empty(&_waiting_threads));
}

/// @brief Wait for the event to signal.
void ipc::event::wait()
{
  timed_wait(ipc::MAX_WAIT);
}

/// @brief Wait for a set period for the event to signal
///
/// @param wait_in_us Maximum waiting time in microseconds
///
/// @return True if the event fired, false otherwise.
bool ipc::event::timed_wait(uint64_t wait_in_us)
{
  bool still_in_wait_list{false};
  task_thread *cur_thread = task_get_cur_thread();

  KL_TRC_ENTRY;

  ASSERT(cur_thread);
  ASSERT(!cur_thread->is_worker_thread);
  klib_list_item<task_thread *> *list_item = new klib_list_item<task_thread *>;
  klib_list_item_initialize(list_item);
  list_item->item = cur_thread;

  ipc_raw_spinlock_lock(this->_list_lock);
  task_continue_this_thread();

  cur_thread->stop_thread();
  klib_list_add_tail(&this->_waiting_threads, list_item);
  ipc_raw_spinlock_unlock(this->_list_lock);

  if (should_still_sleep() && !triggered)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Continue with sleep\n");
    if (wait_in_us != ipc::MAX_WAIT)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Set maximum waiting time");
      cur_thread->wake_thread_after = time_get_system_timer_count() + (wait_in_us * 1000);
    }

    task_resume_scheduling();

    // Having added ourselves to the list we should not pass through task_yield() until the thread is re-awakened below.
    // It is possible that the thread was signalled between the list being unlocked above and here, in which case it is
    // reasonable to just carry on.
    task_yield();

#ifndef AZALEA_TEST_CODE
    // Don't include this section in the unit tests because they don't support task_yield() or timed based resumption
    // of threads which means they get very confused.
    still_in_wait_list = cancel_waiting_thread(cur_thread);
#endif
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Abort sleep\n");

    cur_thread->start_thread();
    task_resume_scheduling();

    cancel_waiting_thread(cur_thread);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Timed out? ", still_in_wait_list, "\n");
  KL_TRC_EXIT;

  return !still_in_wait_list;
}

/// @brief Reset the event to 'unsignalled'
void ipc::event::reset()
{
  ipc_raw_spinlock_lock(this->_list_lock);
  triggered = false;
  ipc_raw_spinlock_unlock(this->_list_lock);
}

/// @brief Called immediately before finally sleeping, give the child class a chance to abort the wait.
///
/// This might be useful if an asynchronous event has happened whilst preparing to sleep.
///
/// @return True if the calling thread should sleep, false otherwise.
bool ipc::event::should_still_sleep()
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;

  return true;
}

/// @brief Called before waking a waiting thread.
///
/// This allows child classes to extend WaitObjects. It is called once the WaitObject's lock has been acquired. It is
/// called once or possibly twice for each thread being woken.
void ipc::event::before_wake_cb()
{
  // No action in the base class.
}

/// @brief Signal that the event has occurred.
///
///
void ipc::event::signal_event()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::FLOW, "Triggering from object: ", this, "\n");

  ipc_raw_spinlock_lock(this->_list_lock);

  triggered = true;

  before_wake_cb();

  while(klib_list_get_length(&this->_waiting_threads) != 0)
  {
    trigger_next_thread(false);
  }
  ipc_raw_spinlock_unlock(this->_list_lock);

  KL_TRC_EXIT;
}

/// @brief Permit the next waiting thread to proceed.
///
/// In the default WaitObject implementation threads are triggered one-by-one, in the same order as which they waited
/// on this WaitObject, but derived classes can choose any implementation. If no threads are waiting, nothing happens.
///
/// @param should_lock Set this to false if this function is being called from a function that already holds
///                    _list_lock. Otherwise, leave as true.
void ipc::event::trigger_next_thread(const bool should_lock)
{
  KL_TRC_ENTRY;

  if (should_lock)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "locking\n");
    ipc_raw_spinlock_lock(this->_list_lock);
  }

  before_wake_cb();

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
    ipc_raw_spinlock_unlock(this->_list_lock);
  }

  KL_TRC_EXIT;
}

/// @brief Cause the parameter thread to resume immediately.
///
/// If the thread is already resumed or is not waiting on this object then this call has no effect.
///
/// @param thread The thread to resume
///
/// There is no indication to the thread that it has resumed prematurely.
///
/// @return True if the thread was found in the list and cancelled. False if the thread was not actually waiting.
bool ipc::event::cancel_waiting_thread(task_thread *thread)
{
  bool found{false};

  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Looking for thread ", thread, "\n");

  klib_list_item<task_thread *> *list_item;
  ipc_raw_spinlock_lock(this->_list_lock);

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

    found = true;
  }

  ipc_raw_spinlock_unlock(this->_list_lock);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", found, "\n");
  KL_TRC_EXIT;

  return found;
}
