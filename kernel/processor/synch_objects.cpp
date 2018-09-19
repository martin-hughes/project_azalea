/// @file
/// @brief Implementation of common synchronization objects that threads may choose to wait on.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/synch_objects.h"

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
void WaitObject::wait_for_signal()
{
  KL_TRC_ENTRY;

  task_thread *cur_thread = task_get_cur_thread();
  klib_list_item<task_thread *> *list_item = new klib_list_item<task_thread *>;
  klib_list_item_initialize(list_item);
  list_item->item = cur_thread;

  klib_synch_spinlock_lock(this->_list_lock);
  task_continue_this_thread();

  cur_thread->stop_thread();
  klib_list_add_tail(&this->_waiting_threads, list_item);
  klib_synch_spinlock_unlock(this->_list_lock);

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
void WaitObject::trigger_next_thread()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(this->_list_lock);

  klib_list_item<task_thread *> *list_item = this->_waiting_threads.head;
  if (list_item != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Starting thread ", list_item->item, "\n");
    klib_list_remove (list_item);
    list_item->item->start_thread();
    delete list_item;
    list_item = nullptr;
  }

  klib_synch_spinlock_unlock(this->_list_lock);

  KL_TRC_EXIT;
}

void WaitObject::trigger_all_threads()
{
  KL_TRC_ENTRY;

  while(klib_list_get_length(&this->_waiting_threads) != 0)
  {
    this->trigger_next_thread();
  }

  KL_TRC_EXIT;
}