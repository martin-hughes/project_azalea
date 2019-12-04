/// @file
/// @brief Message identifiers used in the Azalea Kernel.

#pragma once

#include <stdint.h>
#include <memory>

#include "user_interfaces/messages.h"

namespace msg
{
  /// @brief The root class of all possible messages.
  ///
  /// Being as simple as possible, it only contains a field representing the ID of the message bieng sent.
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
