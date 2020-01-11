/// @file
/// @brief Implements core IDevice functionality.
//
// Known defects:
// - dev_count is system wide rather than per device, which looks a bit odd.
// - There's no testing, but we could test (e.g.) names.

//#define ENABLE_TRACING

#include "device_interface.h"
#include "user_interfaces/messages.h"
#include "klib/klib.h"

#include <atomic>
#include <stdio.h>

/// @brief The number of distinct devices seen by the system.
///
/// This counter increments every time a new device is constructed. It is then used as the suffix if "auto_inc_suffix"
/// is used in the IDevice constructor.
std::atomic<uint64_t> dev_count{0};

IDevice::IDevice(const std::string human_name, const std::string short_name, bool auto_inc_suffix) :
 device_human_name{human_name}, device_short_name{short_name}, current_dev_status{DEV_STATUS::UNKNOWN}
{
  KL_TRC_ENTRY;

  uint64_t dev_number = dev_count++;

  if (auto_inc_suffix)
  {
    char number_buffer[17] = { 0 };
    snprintf(number_buffer, 17, "%lu", dev_number);
    std::string number{number_buffer};
    const std::string true_short_name = short_name + number;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Adding suffix - new device name: ", true_short_name, "\n");
    device_short_name = true_short_name;
  }

  KL_TRC_EXIT;
}

void IDevice::handle_message(std::unique_ptr<msg::root_msg> &message)
{
  msg::basic_msg *bm;
  KL_TRC_ENTRY;


  switch(message->message_id)
  {
  case SM_DEV_START:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Start message\n");
    this->start();
    break;

  case SM_DEV_STOP:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Stop message\n");
    this->stop();
    break;

  case SM_DEV_RESET:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Reset message\n");
    this->reset();
    break;

  case SM_GET_OPTIONS:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Get options structure\n");
    this->get_options_struct(message->output_buffer.get(), message->output_buffer_len);
    break;

  case SM_SET_OPTIONS:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Save options structure\n");
    bm = dynamic_cast<msg::basic_msg *>(message.get());
    if (bm)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Basic message\n");
      this->save_options_struct(bm->details.get(), bm->message_length);
    }
    break;

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Message ", message->message_id, " sent to subclass\n");
    this->handle_private_msg(message);
    break;
  }

  KL_TRC_EXIT;
}

void IDevice::set_device_status(DEV_STATUS new_state)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New state: ", new_state, "\n");
  current_dev_status = new_state;

  KL_TRC_EXIT;
}
