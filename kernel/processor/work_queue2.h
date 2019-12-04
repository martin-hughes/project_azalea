/// @file
/// @brief Implements the main message passing queue in Azalea.

#pragma once

#include <stdint.h>
#include <memory>
#include <queue>
#include "klib/synch/kernel_locks.h"
#include "processor/common_messages.h"

namespace work
{
  void init_queue();

#ifdef AZALEA_TEST_CODE
  void test_only_terminate_queue();
#endif

#ifndef AZALEA_TEST_CODE
  [[noreturn]]
#endif
  void work_queue2_thread();
  void work_queue_one_loop();

  // Forward declarations necessary to create the queue_message function.
  class message_receiver;
  struct message_header;
  void queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> msg);

  /// @brief A simple message-receiving class.
  ///
  /// Any object that wishes to receive messages from the work queue system must inherit from this base class.
  class message_receiver
  {
  public:
    virtual ~message_receiver() = 0;

    // Documentation in work_queue2.cpp
    virtual void begin_processing_msgs();
    virtual bool process_next_message();

  protected:
    message_receiver();

    /// @brief Receive the message contained in message.
    ///
    /// The object should now handle the message contained in msg without blocking. Blocking may cause the system to
    /// deadlock.
    ///
    /// This function will be called by process_next_message(), so should not be called externally.
    ///
    /// @param message The message to handle.
    virtual void handle_message(std::unique_ptr<msg::root_msg> &message) = 0;

  private:
    // ... the message queue storage and handling.

    /// @cond
    // doxygen inexplicably wants to document this whole thing all over again...
    friend
    void work::queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> message);
    /// @endcond
    std::queue<std::unique_ptr<msg::root_msg>> message_queue; ///< The queue of messages stored for this object.
    kernel_spinlock queue_lock; ///< A lock protecting message_queue
    bool in_process_mode{false}; ///< Are we processing message already?

    /// Has this object already been added to the list of objects awaiting message handling?
    ///
    bool is_in_receiver_queue{false};
  };
};
