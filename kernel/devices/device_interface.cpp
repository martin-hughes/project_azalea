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

  register_handler(SM_DEV_START, [this](std::unique_ptr<msg::root_msg> msg){ this->start(); });
  register_handler(SM_DEV_STOP, [this](std::unique_ptr<msg::root_msg> msg){ this->stop(); });
  register_handler(SM_DEV_RESET, [this](std::unique_ptr<msg::root_msg> msg){ this->reset(); });
  register_handler(SM_GET_OPTIONS, [this](std::unique_ptr<msg::root_msg> msg)
    {
      this->get_options_struct(msg->output_buffer.get(), msg->output_buffer_len);
    } );
  register_handler(SM_GET_OPTIONS, [this](std::unique_ptr<msg::root_msg> msg)
    {
      msg::basic_msg *bm;
      bm = dynamic_cast<msg::basic_msg *>(msg.get());
      if (bm)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Basic message\n");
        this->save_options_struct(bm->details.get(), bm->message_length);
      }
    });

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
