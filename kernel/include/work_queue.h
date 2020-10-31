/// @file
/// @brief Implements the main message passing queue in Azalea.

#pragma once

#include <stdint.h>
#include <memory>
#include <queue>
#include <functional>
#include <list>
#include "types/spinlock.h"
#include "types/common_messages.h"

namespace work
{
#ifdef AZALEA_TEST_CODE
  void test_only_terminate_queue();
#endif

#ifndef AZALEA_TEST_CODE
  [[noreturn]]
#endif
  void work_queue_thread();

  // Forward declarations necessary to create the queue_message function. This must be done here to allow it to be a
  // friend later on.
  class message_receiver;
  struct message_header;
  void queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> msg);

  template <class T> using msg_handler = std::function<void(std::unique_ptr<T>)>;

  void ignore(std::unique_ptr<msg::root_msg> msg);
  void bad_conversion(std::unique_ptr<msg::root_msg> msg);

#ifdef AZ_STRICT_MESSAGE_HANDLING
#define FAILED_CONV bad_conversion
#else
#define FAILED_CONV ignore
#endif

  /// @brief A message handler that converts the message to the appropriate type and forwards it to a better handler.
  ///
  /// @param msg The message, as stored in the work queue.
  ///
  /// @param handler Handler of the correct type for the message. This is called if the message can be converted to
  ///                this type.
  ///
  /// @param failure_handler Called if the message cannot be converted to the appropriate type. The default is to
  ///                        panic.
  template <class T> void generic_conversion(std::unique_ptr<msg::root_msg> msg,
                                             msg_handler<T> handler,
                                             msg_handler<msg::root_msg> failure_handler = FAILED_CONV)
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

#define HANDLER(fn) \
    std::function<void(std::unique_ptr<msg::root_msg>)>( \
    [this](std::unique_ptr<msg::root_msg> msg) \
    { \
      this-> fn (std::move(msg)); \
    } )


  /// @brief A simple message-receiving class.
  ///
  /// Any object that wishes to receive messages from the work queue system must inherit from this base class.
  class message_receiver
  {
  public:
    virtual ~message_receiver() = 0;

    // Documentation in work_queue.cpp
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

    /// @brief Register a handler for a given message ID.
    ///
    /// This handler is stored in a map that is referenced by the default handle_message() function. If that function
    /// is overridden then this handler may not be called.
    ///
    /// @param message_id The message ID to register a handler for.
    ///
    /// @param handler A function to call when processing a message with the ID message_id.
    virtual void register_handler(uint64_t message_id, msg_handler<msg::root_msg> handler);

  public:

    /// @cond
    // doxygen inexplicably wants to document this whole thing all over again...
    friend
    void work::queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> message);
    /// @endcond
    std::queue<std::unique_ptr<msg::root_msg>> message_queue; ///< The queue of messages stored for this object.
    ipc::spinlock queue_lock; ///< A lock protecting message_queue

    /// Has this object already been added to the list of objects awaiting message handling? This is quicker and faster
    /// than searching the queue each time to figure out. This should always be protected by queue_lock, above.
    bool is_in_receiver_queue{false};

    /// Maps message IDs to functions that handle those messages.
    std::map<uint64_t, msg_handler<msg::root_msg>> msg_receivers;

    /// Is this handler ready to receive further messages? (For example, it might be waiting to for a device to finish
    /// working before being able to process another request)
    bool ready_to_receive{true};
  };

  // Interface describing the work queue system
  //
  class IWorkQueue
  {
  public:
    virtual ~IWorkQueue() = default;

    virtual void queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> msg) = 0;
    virtual void work_queue_one_loop() = 0;
  };

  //
  class default_work_queue : public IWorkQueue
  {
  public:
    default_work_queue() = default;
    virtual ~default_work_queue() = default;

    virtual void queue_message(std::shared_ptr<message_receiver> receiver, std::unique_ptr<msg::root_msg> msg) override;
    virtual void work_queue_one_loop() override;

  protected:
    /// @brief A list of objects with messages pending.
    std::list<std::weak_ptr<message_receiver>> receiver_queue;

    /// @brief Lock for receiver_queue.
    ipc::spinlock receiver_queue_lock;
  };

  extern IWorkQueue *system_queue;

  /// @brief Initialise the system-wide work queue.
  template <class wq = default_work_queue> void init_queue()
  {
    KL_TRC_ENTRY;

    system_queue = new wq();

    KL_TRC_EXIT;
  }
};
