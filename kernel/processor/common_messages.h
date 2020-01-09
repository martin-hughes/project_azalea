/// @file
/// @brief Message identifiers used in the Azalea Kernel.

#pragma once

#include <stdint.h>
#include <memory>

#include "user_interfaces/messages.h"
#include "processor/synch_objects.h"

namespace msg
{
  /// @brief The root class of all possible messages.
  ///
  /// Being as simple as possible, it only contains a field representing the ID of the message being sent, and an
  /// optional system for signalling that the message has been handled
  class root_msg
  {
  public:
    root_msg() = default; ///< Default constructor.

    /// @brief Constructs and sets the message ID internally.
    ///
    /// @param msg_id The ID of the message being sent.
    root_msg(uint64_t msg_id) : message_id{msg_id} { };
    virtual ~root_msg() = default; ///< Default destructor.

    uint64_t message_id{0}; ///< The ID of the message being sent.

    /// If set to true, when the work queue finishes handling this message, it will signal completion_semaphore (if it
    /// is not nullptr). If false, it is assumed the receiver object will do this.
    ///
    /// This value is intended to be set by the handling object, but in principle it could be set by the message sender
    /// (although it's not clear if this has a useful use case!)
    bool auto_signal_semaphore{true};

    /// A semaphore set by the caller that should be signalled when the message has finished being handled. If
    /// auto_signal_semaphore is set to true, then this will be done by the message handling code, but otherwise it is
    /// the responsibility of the recipient object to do this.
    ///
    /// Correctly coded message senders will be prepared for the possibility this semaphore is never signalled.
    ///
    /// It is permissible for completion_semaphore to be nullptr, in which case it is ignored.
    std::shared_ptr<syscall_semaphore_obj> completion_semaphore{nullptr};

    /// Optional buffer to write results or other similar information in to. The handler should not rely on this
    /// pointer being set correctly, nor of it being the correct size.
    //
    // NB: our current C++ library doesn't natively support shared_ptr to array, so this is a fudge, but if provided a
    // shared_ptr created using a custom deleter so as to support an array, it will work.
    std::shared_ptr<char> output_buffer;

    /// The size of output_buffer. If this is zero, output_buffer must be nullptr. If it is non-zero, output_buffer
    /// must be a valid pointer.
    uint64_t output_buffer_len;
  };

  /// @brief A message that carries a payload of raw bytes.
  ///
  /// This type of message can be used to simulate the way messages would have been sent in a C-style environment: a
  /// structure of type, length, value.
  class basic_msg : public root_msg
  {
  public:
    basic_msg() = default; ///< Default constructor.
    virtual ~basic_msg() override = default; ///< Default destructor.

    uint64_t message_length{0}; ///< The number of bytes stored in details.
    std::unique_ptr<uint8_t[]> details; ///< Storage for the 'value' of the message, as raw bytes.
  };
};
