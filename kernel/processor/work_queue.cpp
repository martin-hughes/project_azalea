/// @file
/// @brief Implementation of work queues in Azalea.
///
/// Sometimes it is useful to allow asynchronous execution of some task or another, and the work queue system allows
/// that. The caller queues a work object, and when it is able, the system executes whatever the work object requested.
/// The caller has the option of waiting for a response, continuing with some other work and looking at the response
/// later, or discarding the response altogether.
///
/// The expectation is that most of the work_* classes will be subclassed by the user. Most likely, the worker_object
/// subclass will be a driver that accepts asynchronous requests, and that class will provide methods that create work
/// request objects that the user can then queue using work::queue_work_item().
///
/// work::queue_work_item() is not a mandatory part of each worker object because it requires a shared pointer to the
/// object that is receiving the work. If it were a part of the worker_object interface, it would probably require each
/// worker_object subclass to derive from std::shared_from_this(), which seems undesirable.

// Known defects:
// - No attempt is made to manage the number of worker threads running - they could, theoretically, propagate for ever.

//#define ENABLE_TRACING

#include "processor/work_queue.h"
#include "klib/klib.h"

#include <queue>

using namespace work;

/// @brief A single item of work waiting to be scheduled.
///
struct queued_item
{
  std::shared_ptr<worker_object> object; ///< The object that will handle the requested work.
  std::shared_ptr<work_item> item; ///< The work item requested.
};

kernel_spinlock work_queue_lock = 0; ///< A simple lock on the main work queue.
uint64_t running_work_threads = 0; ///< The number of work threads currently running.

/// The main work queue itself.
///
/// Created when the work queue thread starts.
std::queue<queued_item> *main_work_queue = nullptr;

#ifdef AZALEA_TEST_CODE
bool stop_work_queue_thread = false;
volatile bool work_queue_thread_complete = true;
#endif

namespace
{
  void start_next_worker_thread();
}

/// @brief Create the work queue storage if it isn't already created.
///
/// This function can be called multiple times in a thread-safe way, so that whichever part of the work queue interface
/// is called first can call it and the queue will then exists.
void maybe_create_work_queue()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(work_queue_lock);
  if (main_work_queue == nullptr)
  {
    main_work_queue = new std::queue<queued_item>();
  }
  klib_synch_spinlock_unlock(work_queue_lock);

  KL_TRC_EXIT;
}

/// @brief The main work queue thread.
///
/// This function is scheduled as a thread by task_create_system_process. It is a very simple loop that executes work
/// items in a linear and not very smart manner.
void work::work_queue_thread()
{
  KL_TRC_ENTRY;

  task_get_cur_thread()->is_worker_thread = true;

#ifdef AZALEA_TEST_CODE
  work_queue_thread_complete = false;
#endif

  if (main_work_queue == nullptr)
  {
    maybe_create_work_queue();
  }

  klib_synch_spinlock_lock(work_queue_lock);
  running_work_threads++;
  klib_synch_spinlock_unlock(work_queue_lock);

  while(1)
  {
    if (main_work_queue->empty())
    {
      //KL_TRC_TRACE(TRC_LVL::FLOW, "Yielding\n");

      task_yield();
    }
    else
    {
      // Dequeue an item and then make sure to release the queue lock before handling it, in case the act of handling
      // the work item causes more work items to be queued.
      KL_TRC_TRACE(TRC_LVL::FLOW, "Grab an item\n");
      klib_synch_spinlock_lock(work_queue_lock);
      queued_item this_item = main_work_queue->front();
      main_work_queue->pop();
      klib_synch_spinlock_unlock(work_queue_lock);

      // Handle the work item and signal any threads waiting for a response that the response is ready.
      this_item.object->handle_work_item(this_item.item);
      if (this_item.item->response_item != nullptr)
      {
        this_item.item->response_item->set_response_complete();
      }
    }

    // Don't keep huge numbers of work threads running.
    klib_synch_spinlock_lock(work_queue_lock);
    if (running_work_threads > 2)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Abandon this work thread\n");
      running_work_threads--;
      klib_synch_spinlock_unlock(work_queue_lock);

      task_get_cur_thread()->destroy_thread();
    }
    else
    {
      //KL_TRC_TRACE(TRC_LVL::FLOW, "Continue with this thread\n");
      klib_synch_spinlock_unlock(work_queue_lock);
    }

#ifdef AZALEA_TEST_CODE
    if (stop_work_queue_thread)
    {
      work_queue_thread_complete = true;
      return;
    }
#endif
  }

  KL_TRC_EXIT;
}

/// @brief Construct the simplest possible work_response.
///
work_response::work_response() : WaitForFirstTriggerObject{}, response_marked_complete{false}
{

}

/// @brief This function is called by the work queue thread when the handling object has handled its work.
///
/// Any threads waiting for the work to be complete are then signalled. It can be called by the handling object if
/// desired, but the object should be aware that it will be called again and any waiting threads will execute
/// immediately.
void work_response::set_response_complete()
{
  KL_TRC_ENTRY;

  response_marked_complete = true;
  this->trigger_all_threads();

  KL_TRC_EXIT;
}

/// @brief Has this response been marked complete by set_response_complete()?
///
/// @return True if the response has been marked complete, false otherwise.
bool work_response::is_response_complete()
{
  return response_marked_complete;
}

/// @brief Make this thread wait until set_response_complete() has been called.
///
/// If set_response_complete() has been called then this function will return immediately.
void work_response::wait_for_response()
{
  bool is_work_thread = task_get_cur_thread()->is_worker_thread;
  KL_TRC_ENTRY;

  if (is_work_thread)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Start new worker thread\n");
    klib_synch_spinlock_lock(work_queue_lock);
    running_work_threads--;
    klib_synch_spinlock_unlock(work_queue_lock);

    start_next_worker_thread();
  }

  this->wait_for_signal();

  if (is_work_thread)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Mark this thread as running again\n");
    klib_synch_spinlock_lock(work_queue_lock);
    running_work_threads++;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Now ", running_work_threads, " work threads\n");
    klib_synch_spinlock_unlock(work_queue_lock);
  }

  KL_TRC_EXIT;
};


/// @brief Queue a work item for execution asynchronously.
///
/// When work::work_queue_thread gets to it, the provided object is given the provided work item to handle.
///
/// @param object The object that will handle the work item.
///
/// @param item The work item that is provided to the worker object to process.
void work::queue_work_item(std::shared_ptr<worker_object> object, std::shared_ptr<work_item> item)
{
  KL_TRC_ENTRY;

  if (main_work_queue == nullptr)
  {
    maybe_create_work_queue();
  }

  queued_item new_item;
  new_item.object = object;
  new_item.item = item;

  klib_synch_spinlock_lock(work_queue_lock);
  main_work_queue->push(new_item);
  klib_synch_spinlock_unlock(work_queue_lock);

  KL_TRC_EXIT;
}

/// @brief Create the simplest work item possible.
///
work_item::work_item()
{
  KL_TRC_ENTRY;

  // There isn't any possibility of responding to this work item - it does even contain any parameters!
  response_item = nullptr;

  KL_TRC_EXIT;
}

#ifdef AZALEA_TEST_CODE
/// @brief Reset the work queue so the next test case gets a fresh one.
///
void test_only_reset_work_queue()
{
  stop_work_queue_thread = true;

  while (!work_queue_thread_complete) { };

  if (main_work_queue != nullptr)
  {
    delete main_work_queue;
  }
  main_work_queue = nullptr;
}
#endif

namespace
{
  void start_next_worker_thread()
  {
    task_thread *cur_thread;
    std::shared_ptr<task_thread> new_thread;
    KL_TRC_ENTRY;

    cur_thread = task_get_cur_thread();
    ASSERT(cur_thread != nullptr);
    ASSERT(cur_thread->is_worker_thread);

    new_thread = task_thread::create(work::work_queue_thread, cur_thread->parent_process);
    new_thread->start_thread();

    KL_TRC_EXIT;
  }
}
