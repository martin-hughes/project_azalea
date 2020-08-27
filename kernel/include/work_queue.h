/// @file
/// @brief Implements the main message passing queue in Azalea.

#pragma once

#include <stdint.h>
#include <memory>
#include <queue>
#include <functional>
#include "types/spinlock.h"
#include "types/common_messages.h"

namespace work
{
  void init_queue();

#ifdef AZALEA_TEST_CODE
  void test_only_terminate_queue();
#endif

#ifndef AZALEA_TEST_CODE
  [[noreturn]]
#endif
  void work_queue_thread();
  void work_queue_one_loop();

  // Forward declarations necessary to create the queue_message function.
  class message_receiver;
  struct message_header;
  void queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> msg);

  template <class T> using msg_handler = std::function<void(std::unique_ptr<T>)>;

  void ignore(std::unique_ptr<msg::root_msg> msg);
  void bad_conversion(std::unique_ptr<msg::root_msg> msg);

  /// @brief A message handler that converts the message to the appropriate type and forwards it to a better handler.
  ///
  /// @param msg The message, as stored in the work queue.
  ///
  /// @param handler Handler of the correct type for the message. This is called if the message can be converted to
  ///                this type.
  ///
  /// @param failure_handler Called if the message cannot be converted to the appropriate type. The default is to
  ///                        simply ignore the message.
  template <class T> void generic_conversion(std::unique_ptr<msg::root_msg> msg,
                                             msg_handler<T> handler,
                                             msg_handler<msg::root_msg> failure_handler = ignore)
  {
    T *type_msg = dynamic_cast<T *>(msg.get());

    if (type_msg)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Successful message conversion\n");
      msg.release();
      std::unique_ptr<T> msg_ptr(type_msg);
      handler(std::move(msg_ptr));
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Failed message conversion\n");
      failure_handler(std::move(msg));
    }
  }

#define DEF_CONVERT_HANDLER(type, fn) \
    std::function<void(std::unique_ptr<msg::root_msg>)>( \
    [this](std::unique_ptr<msg::root_msg> msg) \
    { \
      work::generic_conversion<type>(std::move(msg), [this](std::unique_ptr< type > msg) { this-> fn (std::move(msg)); } ); \
    } )


  /// @brief A simple message-receiving class.
  ///
  /// Any object that wishes to receive messages from the work queue system must inherit from this base class.
  class message_receiver
  {
  public:
    virtual ~message_receiver() = 0;

    // Documentation in work_queue.cpp
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
    virtual void handle_message(std::unique_ptr<msg::root_msg> message);

    virtual void register_handler(uint64_t message_id, msg_handler<msg::root_msg> handler);

  private:
    // ... the message queue storage and handling.

    /// @cond
    // doxygen inexplicably wants to document this whole thing all over again...
    friend
    void work::queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> message);
    /// @endcond
    std::queue<std::unique_ptr<msg::root_msg>> message_queue; ///< The queue of messages stored for this object.
    ipc::raw_spinlock queue_lock; ///< A lock protecting message_queue
    bool in_process_mode{false}; ///< Are we processing message already?

    /// Has this object already been added to the list of objects awaiting message handling?
    ///
    bool is_in_receiver_queue{false};

    /// Maps message IDs to functions that handle those messages.
    std::map<uint64_t, msg_handler<msg::root_msg>> msg_receivers;
  };
};
