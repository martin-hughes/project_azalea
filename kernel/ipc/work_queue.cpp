/// @file
/// @brief Implements the main message passing queue in Azalea.

//#define ENABLE_TRACING

#include "work_queue.h"
#include "processor.h"
#include "map_helpers.h"

#include <list>
#include <mutex>

#ifdef AZALEA_TEST_CODE
#include <thread>
#endif

using namespace work;

namespace
{
  ipc::raw_spinlock queue_ptr_lock{0};
}

namespace work
{
  IWorkQueue *system_queue{nullptr};
}

#ifdef AZALEA_TEST_CODE
/// @brief Terminate the queue for tests, so the tests don't leak memory.
void work::test_only_terminate_queue()
{
  KL_TRC_ENTRY;

  ASSERT(system_queue != nullptr);
  delete system_queue;
  system_queue = nullptr;
  queue_ptr_lock = 0;

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
  ipc_raw_spinlock_lock(queue_ptr_lock);
  if (system_queue == nullptr)
  {
    init_queue<work::default_work_queue>();
  }
  ipc_raw_spinlock_unlock(queue_ptr_lock);

  while(1)
  {
    system_queue->work_queue_one_loop();
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
void work::default_work_queue::work_queue_one_loop()
{
  std::weak_ptr<message_receiver> receiver_wk;
  std::shared_ptr<message_receiver> receiver_str;

  KL_TRC_ENTRY;

  {
    std::unique_lock<ipc::spinlock> receiver_queue_guard(receiver_queue_lock);

    // Attempt to get an object to work on.
    if (receiver_queue.size() != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Retrieve front receiver\n");
      receiver_wk = receiver_queue.front();
      receiver_queue.pop_front();
      receiver_str = receiver_wk.lock();
      if (receiver_str)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Valid receiver, flag removed from queue\n");
        receiver_str->is_in_receiver_queue = false;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "No more work objects\n");
    }
  }

  if (receiver_str)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Work on ", receiver_str.get(), "\n");
    bool should_continue{receiver_str->ready_to_receive};
    while(should_continue)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "One more message\n");
      should_continue = receiver_str->process_next_message() && receiver_str->ready_to_receive;
    }

    // Check if we need to queue this object again.
    std::unique_lock<ipc::spinlock> queue_lock(receiver_str->queue_lock);
    std::unique_lock<ipc::spinlock> receiver_queue_guard(receiver_queue_lock);

    if (!receiver_str->is_in_receiver_queue && !receiver_str->message_queue.empty())
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Outstanding messages - requeue\n");

      receiver_queue.push_back(receiver_str);
      receiver_str->is_in_receiver_queue = true;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No work to do\n");
    task_yield();
  }

  KL_TRC_EXIT;
}

void work::queue_message(std::shared_ptr<work::message_receiver> receiver, std::unique_ptr<msg::root_msg> msg)
{
  KL_TRC_ENTRY;

  ASSERT(system_queue);
  system_queue->queue_message(receiver, std::move(msg));

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
void work::default_work_queue::queue_message(std::shared_ptr<work::message_receiver> receiver,
                                             std::unique_ptr<msg::root_msg> msg)
{
  KL_TRC_ENTRY;

  ASSERT(receiver);
  ASSERT(msg);

  std::unique_lock<ipc::spinlock> guard(receiver->queue_lock);

  ASSERT(msg.get());

  receiver->message_queue.push(std::move(msg));

  std::unique_lock<ipc::spinlock> receiver_queue_guard(receiver_queue_lock);

  if (!receiver->is_in_receiver_queue)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Queue this object for later handling\n");
    receiver_queue.push_back(receiver);
    receiver->is_in_receiver_queue = true;
  }

  KL_TRC_EXIT;
}

/// @brief Standard constructor.
message_receiver::message_receiver()
{
  KL_TRC_ENTRY;

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
  std::shared_ptr<ipc::semaphore> completion_sem{nullptr};

  KL_TRC_ENTRY;

  queue_lock.lock();

  more_msgs = (message_queue.size() > 1);
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of messages: ", message_queue.size(), "\n");

  if (!message_queue.empty())
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Queue not empty\n");
    msg_header = std::move(message_queue.front());
    ASSERT(msg_header.get());
    message_queue.pop();

    queue_lock.unlock();

    if (msg_header->auto_signal_semaphore && msg_header->completion_semaphore)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Save the completion semaphore\n");
      completion_sem = msg_header->completion_semaphore;
    }

    // After this point, we should assume the message to be invalid, as certain conversions done by receivers can cause
    // the message to become invalid. (For example, the device manager releases the message pointer in order to cast it
    // to a different type.)
    this->handle_message(std::move(msg_header));

    if (completion_sem)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Signal completion semaphore\n");
      completion_sem->clear();
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Actually, no more messages\n");
    queue_lock.unlock();
    ASSERT(!more_msgs)
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", more_msgs, "\n");
  KL_TRC_EXIT;

  return more_msgs;
}

void message_receiver::register_handler(uint64_t message_id, work::msg_handler<msg::root_msg> handler)
{
  KL_TRC_ENTRY;

  msg_receivers.insert_or_assign(message_id, handler);

  KL_TRC_EXIT;
}

void message_receiver::handle_message(std::unique_ptr<msg::root_msg> message)
{
  KL_TRC_ENTRY;

  if (map_contains(msg_receivers, message->message_id))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Found item\n");
    msg_handler<msg::root_msg> handler = msg_receivers[message->message_id];
    handler(std::move(message));
  }
  else
  {
    // For now we ignore the message. In future, there might be a different action.
    KL_TRC_TRACE(TRC_LVL::FLOW, "Didn't find a handler\n");
#ifdef AZ_STRICT_MESSAGE_HANDLING
    panic("Didn't find message handler");
#endif
  }

  KL_TRC_EXIT;
}

/// @brief Does nothing with this message.
///
/// @param msg This message is ignored.
void work::ignore(std::unique_ptr<msg::root_msg> msg)
{

}

/// @brief Causes a panic.
///
/// @param msg This parameter is ignored.
void work::bad_conversion(std::unique_ptr<msg::root_msg> msg)
{
  panic("Failed message type conversion");
}
