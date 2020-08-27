/// @file
/// @brief Implements basic IO Object functionality.

//#define ENABLE_TRACING

#include "types/io_object.h"

using REQS = msg::io_msg::REQS;

IIOObject::IIOObject()
{
  KL_TRC_ENTRY;

  register_handler(SM_IO_MSG, DEF_CONVERT_HANDLER(msg::io_msg, handle_io_request));

  KL_TRC_EXIT;
}

void IIOObject::handle_io_request(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::FLOW, "Received IO message.\n");

  switch (msg->request)
  {
  case REQS::READ:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Read request.\n");
    read(std::move(msg));
    break;

  case REQS::WRITE:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Write request.\n");
    write(std::move(msg));
    break;

  case REQS::INVALID:
  default:
    panic("Invalid I/O request.");
    break;
  }

  KL_TRC_EXIT;
}

void IIOObject::default_handler(std::unique_ptr<msg::io_msg> msg)
{
  INCOMPLETE_CODE(default_handler);
}

void IIOObject::complete_io_request(std::unique_ptr<msg::io_msg> msg)
{
  KL_TRC_ENTRY;

  msg->message_id = SM_IO_COMPLETE;
  std::shared_ptr<work::message_receiver> sender = msg->sender.lock();

  if (sender)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Sender still exists\n");
    work::queue_message(sender, std::move(msg));
  }

  KL_TRC_EXIT;
}
