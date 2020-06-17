/// @file
/// @brief Monitors attached devices and attempts to keep them running as much as possible.

#pragma once

#include <memory>

#include "k_assert.h"
#include "panic.h"

#include "work_queue.h"
#include "azalea/messages.h"
#include "types/device_interface.h"

// Forward declare device types, to save including a whole bunch of unrelated headers.
class IDevice;
class generic_keyboard;
namespace terms
{
  class generic;
};
class IBlockDevice;

namespace dev
{

/// @brief Message indicating that a new device needs registering with the Device Monitor.
///
class dev_reg_msg : public msg::root_msg
{
public:
  dev_reg_msg() : msg::root_msg{SM_DEV_REGISTER} { }; ///< Default constructor.
  virtual ~dev_reg_msg() override = default; ///< Default destructor.

  std::shared_ptr<IDevice> dev; ///< The device that requires registering.
};

/// @brief Device Monitor tracks all devices in the system.
///
/// Whilst at the moment it is a pretty convoluted way of just keeping references to known devices, the aim for the
/// future is for it to reset failed devices and send appropriate messages to interested parties.
///
/// Only a single instance is present on a running Azalea system.
class monitor : public work::message_receiver
{
public:
  virtual ~monitor();

/// @cond
  monitor(const monitor &) = delete;
  monitor& operator=(const monitor &) = delete;
/// @endcond

  static void init();
#ifdef AZALEA_TEST_CODE
  static void terminate();
#endif
  static bool register_device(std::shared_ptr<IDevice> &new_dev);

protected:
  virtual void handle_message(std::unique_ptr<msg::root_msg> &msg) override;
  virtual void handle_register(std::unique_ptr<dev_reg_msg> &msg);

  std::vector<std::shared_ptr<IDevice>> registered_devices; ///< All known devices in the system.
  struct
  {
    std::vector<std::shared_ptr<generic_keyboard>> keyboards; ///< A list of all keyboards found in the system.
    std::vector<std::shared_ptr<terms::generic>> terminals; ///< A list of all terminals found in the system.
    std::vector<std::shared_ptr<IBlockDevice>> block_devices; ///< A list of all block devices found in the system.
  } devs_by_type; ///< Devices grouped by their type.


private:
  monitor();
};

/// @brief Create a new device object and register it with dev_monitor.
///
/// @tparam t The type of deviec to create.
///
/// @tparam Args Other arguments that get passed to the constructor
///
/// @param[out] new_dev Pointer to the newly created device.
///
/// @param[in] parent Optional pointer to the parent device.
///
/// @param[in] args Other arguments to pass to the device's constructor.
///
/// @return True if the device was successfully registered. False otherwise. This value doesn't indicate whether or not
///         the new device is healthy.
template <typename t, typename... Args>
bool create_new_device(std::shared_ptr<t> &new_dev, std::shared_ptr<IDevice> &parent, Args... args)
  // Note: The declaration of this function also appears in two places in device_interface.h that must be kept in sync.
{
  bool result{true};
  std::shared_ptr<IDevice> dev_base;

  KL_TRC_ENTRY;

  new_dev = std::make_shared<t>(args...);
  dev_base = std::dynamic_pointer_cast<IDevice>(new_dev);
  ASSERT(dev_base);
  dev_base->self_weak_ptr = dev_base;
  result = dev::monitor::register_device(dev_base);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

};
