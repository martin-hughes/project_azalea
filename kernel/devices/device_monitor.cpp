/// @file
/// @brief Implements the dev_monitor system.
///
/// dev_monitor tracks the health of all devices in the system and attempts to keep them as alive as possible.

//#define ENABLE_TRACING

#include "device_monitor.h"
#include "klib/klib.h"

#include "devices/generic/gen_keyboard.h"
#include "devices/generic/gen_terminal.h"
#include "devices/block/block_interface.h"
#include "system_tree/system_tree.h"

namespace
{
std::shared_ptr<dev::monitor> *dev_monitor{nullptr};
}

namespace dev
{

/// @brief Construct the system's dev_monitor
///
dev::monitor::monitor()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Destroy the system's dev_monitor. This should only really happen in testing.
///
dev::monitor::~monitor()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Initialise dev_monitor.
///
void dev::monitor::init()
{
  KL_TRC_ENTRY;

  ASSERT(dev_monitor == nullptr);
  dev::monitor *obj = new dev::monitor();
  dev_monitor = new std::shared_ptr<dev::monitor>(obj);

  std::shared_ptr<system_tree_simple_branch> all_branch = std::make_shared<system_tree_simple_branch>();
  std::shared_ptr<system_tree_simple_branch> keyboard_branch = std::make_shared<system_tree_simple_branch>();
  std::shared_ptr<system_tree_simple_branch> term_branch = std::make_shared<system_tree_simple_branch>();
  std::shared_ptr<system_tree_simple_branch> block_branch = std::make_shared<system_tree_simple_branch>();

  ASSERT(system_tree()->add_child("\\dev\\all", all_branch) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child("\\dev\\keyb", keyboard_branch) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child("\\dev\\term", term_branch) == ERR_CODE::NO_ERROR);
  ASSERT(system_tree()->add_child("\\dev\\block", block_branch) == ERR_CODE::NO_ERROR);

  KL_TRC_EXIT;
}

#ifdef AZALEA_TEST_CODE
/// @brief Reset dev_monitor at the end of a test.
///
void dev::monitor::terminate()
{
  KL_TRC_ENTRY;

  ASSERT(dev_monitor != nullptr);

  *dev_monitor = nullptr;
  delete dev_monitor;
  dev_monitor = nullptr;

  KL_TRC_EXIT;
}
#endif

/// @brief Register a new device with dev_monitor.
///
/// dev_monitor will then take care of starting the device and tracking it through its lifespan.
///
/// @param new_dev The device to register with dev_mon.
///
/// @return True if the device was successfully registered. False otherwise. This value doesn't indicate whether or not
///         the new device is healthy.
bool dev::monitor::register_device(std::shared_ptr<IDevice> &new_dev)
{
  bool result{true};

  KL_TRC_ENTRY;

  if(!new_dev)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not valid device\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send register message\n");

    std::unique_ptr<dev_reg_msg> message = std::make_unique<dev_reg_msg>();
    message->dev = new_dev;
    ASSERT(message->message_id == SM_DEV_REGISTER);
    ASSERT((dev_monitor != nullptr) && (*dev_monitor));
    ASSERT(message.get());
    work::queue_message(*dev_monitor, std::move(message));
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @cond
// Don't document this internal-only macro.
#define HANDLE_MSG(msg, id, obj_type, handler) \
  case (id): \
    { \
      KL_TRC_TRACE(TRC_LVL::FLOW, "Handle ", # id, "\n"); \
      std::unique_ptr< obj_type > msg_c(dynamic_cast< obj_type *>(msg.release())); \
      this-> handler (msg_c); \
    } \
    break;
/// @endcond

/// @brief Handle device related messages.
///
/// @param message Message to handle. Exact list TBD.
void dev::monitor::handle_message(std::unique_ptr<msg::root_msg> &message)
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Process message ID: ", message->message_id, "\n");

  // Note that after this switch message, msg will no longer be valid as HANDLE_MSG() releases it.
  switch(message->message_id)
  {
  HANDLE_MSG(message, SM_DEV_REGISTER, dev::dev_reg_msg, handle_register);

  default:
    KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown message ID: ", message->message_id, "\n");
  }

  KL_TRC_EXIT;
}

/// @cond
// Don't document internal macro
#define REG_TYPE(type, list, subtree) \
{ \
  std::shared_ptr< type > p = std::dynamic_pointer_cast< type >(msg->dev); \
  if (p) \
  { \
    KL_TRC_TRACE(TRC_LVL::FLOW, "Register device of type " # type "\n"); \
    this->devs_by_type. list .push_back(p); \
    path = "\\dev\\" subtree "\\"; \
    path = path + msg->dev->dev_short_name(); \
    KL_TRC_TRACE(TRC_LVL::FLOW, "Add new device path: ", path, "\n"); \
    ASSERT(system_tree()->add_child(path, msg->dev) == ERR_CODE::NO_ERROR); \
  } \
}
/// @endcond

/// @brief Handle a device registration message.
///
/// Add this device to internal structures and attempt to start it.
///
/// @param msg Registration message.
void dev::monitor::handle_register(std::unique_ptr<dev_reg_msg> &msg)
{
  std::string path;
  KL_TRC_ENTRY;

  registered_devices.push_back(msg->dev);
  path = "\\dev\\all\\";
  path = path + msg->dev->dev_short_name();
  KL_TRC_TRACE(TRC_LVL::FLOW, "Add new device path: ", path, "\n");
  ASSERT(system_tree()->add_child(path, msg->dev) == ERR_CODE::NO_ERROR);

  // If the device is of the specific type, add it to a list of devices of that type.
  REG_TYPE(generic_keyboard, keyboards, "keyb");
  REG_TYPE(terms::generic, terminals, "term");
  REG_TYPE(IBlockDevice, block_devices, "block");

  std::unique_ptr<msg::root_msg> start_msg = std::make_unique<msg::root_msg>(SM_DEV_START);

  work::queue_message(msg->dev, std::move(start_msg));

  KL_TRC_EXIT;
}

};
