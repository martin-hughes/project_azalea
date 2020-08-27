/// @file
/// @brief A generic IO object. This could be a file, a device, or something more fancy.

#pragma once

#include "work_queue.h"
#include "common_messages.h"

#include <memory>

class IIOObject : public virtual work::message_receiver
{
public:
  IIOObject();
  virtual ~IIOObject() = default;

  IIOObject(const IIOObject &) = delete;
  IIOObject(IIOObject &&) = delete;
  IIOObject &operator=(const IIOObject &) = delete;

  virtual void handle_io_request(std::unique_ptr<msg::io_msg> msg);
  virtual void complete_io_request(std::unique_ptr<msg::io_msg> msg);

  virtual void read(std::unique_ptr<msg::io_msg> msg) { default_handler(std::move(msg)); };
  virtual void write(std::unique_ptr<msg::io_msg> msg) { default_handler(std::move(msg)); };

private:
  void default_handler(std::unique_ptr<msg::io_msg> msg);
};
