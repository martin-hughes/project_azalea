/// @file
/// @brief Implements the main message passing queue in Azalea.

//#define ENABLE_TRACING

#include "work_queue.h"
#include "processor.h"
#include "klib/klib.h"

#include <list>

#ifdef AZALEA_TEST_CODE
#include <thread>
#endif

using namespace work;

namespace
{
  /// @brief A list of objects with messages pending.
  std::list<std::weak_ptr<message_receiver>> *receiver_queue = nullptr;

  /// @brief Lock for receiver_queue.
  kernel_spinlock receiver_queue_lock = 0;
}

/// @brief Initialise the system-wide work queue.
void work::init_queue()
{
  KL_TRC_ENTRY;

  ASSERT(!receiver_queue);
  receiver_queue = new std::list<std::weak_ptr<message_receiver>>();

  KL_TRC_EXIT;
}

#ifdef AZALEA_TEST_CODE
/// @brief Terminate the queue for tests, so the tests don't leak memory.
void work::test_only_terminate_queue()
{
  KL_TRC_ENTRY;

  ASSERT(receiver_queue != nullptr);
  delete receiver_queue;
  receiver_queue = nullptr;
  receiver_queue_lock = 0;

  KL_TRC_EXIT;
}
#endif

#ifdef AZALEA_TEST_CODE
bool test_exit_work_queue{false};
#endif

/// @brief Runs the main work queue. There will be one thread per-CPU.
///
#ifndef AZALEA_TEST_CODE
[[noreturn]]
#endif
void work::work_queue_thread()
{
  KL_TRC_ENTRY;

  // Ensure the scheduler and synchronisation systems know this is a work thread, so they don't allow blocking
  // operations.
  task_get_cur_thread()->is_worker_thread = true;

  // Construct the queue of objects requiring servicing, it it doesn't exist.
  klib_synch_spinlock_lock(receiver_queue_lock);
  if (receiver_queue == nullptr)
  {
    init_queue();
  }
  klib_synch_spinlock_unlock(receiver_queue_lock);

  while(1)
  {
    work_queue_one_loop();
#ifdef AZALEA_TEST_CODE
    if (test_exit_work_queue)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Exit from work queue\n");
      return;
    }
#endif
  }

  // Won't get here:
  //KL_TRC_EXIT;
}

/// @brief The main work loop.
///
/// - Retrieve an object from the from of the queue.
/// - Handle any messages destined for that object.
/// - Move to the next object.
/// - If there are no messages, wait.
void work::work_queue_one_loop()
{
  std::weak_ptr<message_receiver> receiver_wk;
  std::shared_ptr<message_receiver> receiver_str;

  KL_TRC_ENTRY;

  // Attempt to get an object to work on.
  klib_synch_spinlock_lock(receiver_queue_lock);
  if (receiver_queue->size() != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve front receiver\n");
    receiver_wk = receiver_queue->front();
    receiver_queue->pop_front();
    receiver_str = receiver_wk.lock();
    if (receiver_str)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Got receiver\n");
      receiver_str->begin_processing_msgs();
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No more work objects\n");
  }
  klib_synch_spinlock_unlock(receiver_queue_lock);

  if (receiver_str)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Work on ", receiver_str.get(), "\n");
    while(receiver_str->process_next_message()) { };
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No work to do\n");
    task_yield();
  }

  KL_TRC_EXIT;
}

/// @brief Queue a message for later handling by this object.
///
/// It is very unlikely that child classes need to override this function.
///
/// The work_queue_thread will handle this in due course.
///
/// @param receiver The object that should handle this message.
///
/// @param msg The message being sent.
void work::queue_message(std::shared_ptr<work::message_receiver> receiver, std::unique_ptr<msg::root_msg> msg)
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(receiver->queue_lock);

  receiver->message_queue.push(std::move(msg));
  if (!receiver->is_in_receiver_queue && !receiver->in_process_mode)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Queue this object for later handling\n");
    klib_synch_spinlock_lock(receiver_queue_lock);
    receiver_queue->push_back(receiver);
    receiver->is_in_receiver_queue = true;
    klib_synch_spinlock_unlock(receiver_queue_lock);
  }

  klib_synch_spinlock_unlock(receiver->queue_lock);

  KL_TRC_EXIT;
}

/// @brief Standard constructor.
message_receiver::message_receiver()
{
  KL_TRC_ENTRY;

  klib_synch_spinlock_init(queue_lock);

  KL_TRC_EXIT;
}

/// @brief Standard destructor.
message_receiver::~message_receiver()
{

}

/// @brief Handle the next message in this object's queue.
///
/// The message will then be de-queued and freed.
///
/// It is very unlikely that child classes need to override this function.
///
/// @return True if further messages remain in the queue, false otherwise.
bool message_receiver::process_next_message()
{
  bool more_msgs{true};
  std::unique_ptr<msg::root_msg> msg_header;
  std::shared_ptr<syscall_semaphore_obj> completion_sem;

  KL_TRC_ENTRY;

  klib_synch_spinlock_lock(queue_lock);

  if (!message_queue.empty())
  {
    msg_header = std::move(message_queue.front());
    message_queue.pop();

    if (message_queue.empty())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more messages\n");
      in_process_mode = false;
      more_msgs = false;
    }

    klib_synch_spinlock_unlock(queue_lock);

    if (msg_header->auto_signal_semaphore && msg_header->completion_semaphore)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Save the completion semaphore\n");
      completion_sem = msg_header->completion_semaphore;
    }

    // After this point, we should assume the message to be invalid, as certain conversions done by receivers can cause
    // the message to become invalid. (For example, the device manager releases the message pointer in order to cast it
    // to a different type.)
    this->handle_message(msg_header);

    if (completion_sem)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Signal completion semaphore\n");
      completion_sem->signal();
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::IMPORTANT, "No messages waiting - function called in error\n");
    more_msgs = false;
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", more_msgs, "\n");
  KL_TRC_EXIT;

  return more_msgs;
}

/// @brief This must be called by the work system before any messages are dispatched.
///
/// It must not be called otherwise.
void message_receiver::begin_processing_msgs()
{
  KL_TRC_ENTRY;

  // This doesn't guarantee that we're the thread owning the lock, but over time if there's a bug then we should hit
  // this assert by statistics.
  ASSERT(receiver_queue_lock != 0);

  klib_synch_spinlock_lock(queue_lock);
  is_in_receiver_queue = false;
  in_process_mode = true;
  klib_synch_spinlock_unlock(queue_lock);

  KL_TRC_EXIT;
}
