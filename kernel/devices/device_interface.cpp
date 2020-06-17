/// @file
/// @brief Implements core IDevice functionality.
//
// Known defects:
// - There's no testing, but we could test (e.g.) names.

//#define ENABLE_TRACING

#include <atomic>
#include <map>
#include <stdio.h>

#include "types/device_interface.h"
#include "azalea/messages.h"
#include "map_helpers.h"

namespace
{
  /// @brief Stores the number of devices using a given name in the system.
  ///
  /// This allows devices using `auto_inc_suffix` to increment predictably, for example going through COM1, COM2, etc.
  std::map<std::string, uint64_t> *name_counts{nullptr};

  /// Lock to protect `name_counts`
  ipc::raw_spinlock name_count_lock{0};
}

IDevice::IDevice(const std::string human_name, const std::string short_name, bool auto_inc_suffix) :
 device_human_name{human_name}, device_short_name{short_name}, current_dev_status{OPER_STATUS::UNKNOWN}
{
  uint64_t dev_number{0};

  KL_TRC_ENTRY;

  if (auto_inc_suffix)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Add automatic suffix number\n");

    ipc_raw_spinlock_lock(name_count_lock);

    if (name_counts == nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Construct name_counts object\n");
      name_counts = new std::map<std::string, uint64_t>();
    }

    if (!map_contains(*name_counts, short_name))
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Add ", short_name, " to name_counts\n");
      name_counts->insert({short_name, 0});
    }

    dev_number = ++((*name_counts)[short_name]);

    char number_buffer[17] = { 0 };
    snprintf(number_buffer, 17, "%lu", dev_number);
    std::string number{number_buffer};
    const std::string true_short_name = short_name + number;
    KL_TRC_TRACE(TRC_LVL::FLOW, "Adding suffix - new device name: ", true_short_name, "\n");
    device_short_name = true_short_name;

    ipc_raw_spinlock_unlock(name_count_lock);
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

void IDevice::set_device_status(OPER_STATUS new_state)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "New state: ", new_state, "\n");
  current_dev_status = new_state;

  KL_TRC_EXIT;
}

#ifdef AZALEA_TEST_CODE
void test_only_reset_name_counts()
{
  if (name_counts)
  {
    delete name_counts;
    name_counts = nullptr;
  }
}
#endif
