/// @file
/// @brief Generic device driver interfaces.
///
/// All device drivers must inherit from IDevice, other generic but widely used behaviours have their own interfaces.
///
/// The basic IDevice state machine. Not present is currently not supported.
///
///   *   | Unknown | Failed | Not present | Reset | Stopped | starting | stopping | OK
/// Startup -->|        |          |           |        |          |          |       |
///            |-------Optional self-configuration----->|          |          |       |
///            |---------------------------start()---------------->|          |       |
///            |        |          |           |        |-start()->|          |       |
///            |        |          |           |        |          |-startup complete>|
///            |        |          |           |        |          |--stop()->|       |
///            |        |          |           |        |          |          |<stop()|
///            |        |          |           |        |<--stop complete-----|       |
///            |----------reset()------------->|        |          |          |       |
///            |        |-----reset()--------->|        |          |          |       |
///            |        |          |           |<reset()|          |          |       |
///            |        |          |           |<-----reset()------|          |       |
///            |        |          |           |<-----------reset()-----------|       |
///            |        |          |           |--done->|          |          |       |
/// Any failure ------->|          |           |        |          |          |       |
///            |        |          |           |        |          |          |       |
/// Unscheduled removal of device->|           |        |          |          |       |
///            |        |          |           |        |          |          |       |
/// Device reinserted after removal:           |        |          |          |       |
///   \------->|        |          |           |        |          |          |       |
///            |        |          |           |        |          |          |       |
/// Fatal transition request:      |           |        |          |          |       |
///   \---------------->|          |           |        |          |          |       |
///
/// Entries that are in the * column can happen in any state, and move the state machine immediately into the indicated
/// state.
///
/// In essence:
/// - start(). Move device to starting state.
///    Unknown: valid.
///    Failed: not valid - use reset. Fatal request (moot really!)
///    Not present: not valid. State remains Not Present.
///    Reset: not valid - wait for device to reach stopped. Fatal request.
///    Stopped: valid.
///    Starting: not valid - device is starting. Request ignored
///    Stopping: not valid - wait for device to reach stopped. Fatal request.
///    OK: not valid - device is started. Request ignored.
/// - stop(). Move device to stopping state.
///    Unknown: not valid. Request ignored
///    Failed: not valid - use reset. Fatal request (moot)
///    Not present: not valid. State remains Not Present.
///    Reset: not valid - Wait for device to reach stopped. Fatal request.
///    Stopped: not valid. Request ignored.
///    Starting: not valid - wait for device to reach started. Fatal request.
///    Stopping: not valid - device is already stopping. Fatal request
///    OK: valid.
/// - reset(). Move device to reset state.
///    Unknown: valid.
///    Failed: valid.
///    Not present: not valid. State remains not present.
///    Reset: valid - reset begins again.
///    Stopped: valid.
///    Starting: valid - device will abort start.
///    Stopping: valid - device will reset - maybe a more aggressive stop.
///    OK: not valid - device is started. Fatal request.

#ifndef CORE_DEVICE_INTFACE_HEADER
#define CORE_DEVICE_INTFACE_HEADER

#include <string>
#include <stdint.h>

#include "processor/work_queue.h"
#include "system_tree/system_tree_simple_branch.h"

// Forward declaration to allow the create_new_device forward declaration.
class IDevice;

// used for the friend declaration below, defined in device_monitor.h
namespace dev
{
  template <typename t, typename... Args>
  bool create_new_device(std::shared_ptr<t> &new_dev, std::shared_ptr<IDevice> &parent, Args... args);
};

/// @brief Map OPER_STATUS to the old DEV_STATUS
///
/// DEV_STATUS is the old name, used in too many places to make it worth changing at the moment.
typedef OPER_STATUS DEV_STATUS;

/// @brief The interface that all device drivers must inherit from.
///
/// This doesn't do much at the moment apart from indicating that the child class is a device driver, but expect it to
/// expand in future.
///
/// IDevice inherits from work::message_receiver because all devices must be capable of receiving management messages.
/// It inherits from system_tree_simple_branch to allow devices to have child devices associated with them. Childen of
/// IDevice may choose to override the system_tree_simple_branch features to enable more complex functionality.
class IDevice : public work::message_receiver, public system_tree_simple_branch
{
public:
  /// @brief Standard constructor
  ///
  /// Human names are friendly names like "USB Mouse" or "MiscCorp DooDad". Short names are names used within the dev
  /// filesystem, and are likely to be things like "mouse001" or "doodadx3". Human names can be duplicated - a system
  /// may have more than one USB Mouse. Short names cannot be duplicated, a system can only have one mouse001.
  ///
  /// @param human_name The human friendly name for this device.
  ///
  /// @param short_name The short name for this device.
  ///
  /// @param auto_inc_suffix If set to true, the constructor will append a number to the end of short_name equal to the
  ///                        number of devices already using short_name in the system.
  IDevice(const std::string human_name, const std::string short_name, bool auto_inc_suffix);
  virtual ~IDevice() = default;

  /// @brief Return a human readable name for this device.
  ///
  /// It is acceptable for more than one device object to have the same human readable name.
  ///
  /// @return The human readable name.
  virtual const std::string device_name() { return device_human_name; };

  /// @brief Return a short name for this device.
  ///
  /// Short names are effectively filenames in the dev filesystem. They cannot be duplicated, each instantiation of
  /// this device must have a different short name.
  ///
  /// @return The device's short name.
  virtual const std::string dev_short_name() { return device_short_name; };

  /// @brief Return the current status of this device.
  ///
  /// @return One of DEV_STATUS, as appropriate.
  virtual DEV_STATUS get_device_status() { return current_dev_status; };

  // Override of the message_receiver base class.
  virtual void handle_message(std::unique_ptr<msg::root_msg> &message) override;

  /// @brief Populate a structure with the contents of a device-specific options-structure.
  ///
  /// @param[out] struct_ptr Storage space for the device-specific options structure.
  ///
  /// @param[in] buffer_length Size of the buffer pointed to by struct_ptr.
  ///
  /// @return True if the options structure was written out to struct_ptr successfully, false otherwise.
  virtual bool get_options_struct(void *struct_ptr, uint64_t buffer_length) { return true; };

  /// @brief Save device specific options from a provided structure.
  ///
  /// @param struct_ptr Pointer to the options structure to save.
  ///
  /// @param buffer_length Size of the buffer pointed to by struct_ptr.
  ///
  /// @return True if the options were successfully saved, false otherwise.
  virtual bool save_options_struct(void *struct_ptr, uint64_t buffer_length) { return true; };

protected:
  //
  // Device state machine message handlers:
  //

  /// @brief Trigger any actions to move the device into the starting state.
  ///
  /// This function is responsible for calling set_device_status() as appropriate.
  ///
  /// @return True if the request was valid in the device's current state. False otherwise. Returning false will be
  ///         treated as a fatal transition request.
  virtual bool start() = 0;

  /// @brief Trigger any actions to move the device into the stopping state.
  ///
  /// This function is responsible for calling set_device_status() as appropriate.
  ///
  /// @return True if the request was valid in the device's current state. False otherwise. Returning false will be
  ///         treated as a fatal transition request.
  virtual bool stop() = 0;

  /// @brief Trigger any actions to move the device into the reset state.
  ///
  /// This function is responsible for calling set_device_status() as appropriate.
  ///
  /// @return True if the request was valid in the device's current state. False otherwise. Returning false will be
  ///         treated as a fatal transition request.
  virtual bool reset() = 0;

  /// @brief Device state machine state changer
  ///
  /// This function must be called in order to update the state of the device, so that the device management system
  /// can, for example, schedule a restart of a failed device.
  ///
  /// @param new_state The new state of the device.
  virtual void set_device_status(DEV_STATUS new_state) final;

  /// @brief Handle messages that aren't handled by the IDevice top level object.
  ///
  /// This will include all messages defined by any child of the IDevice object. Users can override
  /// IDevice::handle_message() if they wish, but they will then need to arrange calls to start/stop/etc.
  ///
  /// @param message The message header of the message to handle.
  virtual void handle_private_msg(std::unique_ptr<msg::root_msg> &message) { };

  /// @brief Weak pointer to self.
  ///
  /// This is useful because it means that, provided the object is not being destroyed, a shared_ptr can always be
  /// constructed for this object.
  std::weak_ptr<IDevice> self_weak_ptr;

  /// @cond
  // doxygen inexplicably wants to document this whole thing all over again...
  template <typename t, typename... Args> friend
  bool dev::create_new_device(std::shared_ptr<t> &new_dev, std::shared_ptr<IDevice> &parent, Args... args);
  /// @endcond

private:
  const std::string device_human_name; ///< The human-friendly name for this device.

  /// @brief The short-name for this device.
  ///
  /// Ideally this would be const, but at the moment the constructor adds a suffix if needed.
  std::string device_short_name;
  DEV_STATUS current_dev_status; ///< The current status of this device.
};

/// @brief An interface that must be inherited by all drivers that handle interrupts.
///
/// The receiver is split in to two parts - fast and slow paths. The fast path is always executed by the global
/// interrupt handler, and runs as part of the interrupt handling code. Interrupts are disabled while it runs. The slow
/// path runs as part of a normal kernel thread, and is optional - it is called if the fast path requests it to be.
///
/// The fast path cannot call any part of the kernel that locks - which is quite a lot of it! The slow path can call
/// anything it needs to.
///
/// This interface can also be used by classes that handle IRQs, in which case the interrupt number is replaced by an
/// IRQ when calling member functions.
///
/// More than one device may opt to handle the same interrupt.
class IInterruptReceiver
{
public:
  virtual ~IInterruptReceiver() = default;

  /// @brief The first pass of handling an interrupt. This is called while the interrupt itself is still being handled.
  ///
  /// The system has received an interrupt and determined that this object was registered as a handler. Since each
  /// object could, in principle, be registered for more than one interrupt the system passes the interrupt number to
  /// the handler. No/ other data is passed since the system doesn't know what data it should be passing!
  ///
  /// Since the system is running inside an interrupt handler care should be taken not to run for too long, since this
  /// processor cannot execute another task until the whole interrupt handler is complete.
  ///
  /// Additionally, this function *MUST NOT* lock at all, since it may be called while the lock is owned by a normal,
  /// non-interrupt thread. In that case, the interrupt handler would never return, since the interrupt would be unable
  /// to gain the lock and the non-interrupt thread unable to release it. Note that some common functions DO lock -
  /// e.g. `new`/`kmalloc`.
  ///
  /// Should the owner of this receiver need to do anything involving a lock, return true to indicate that the slow
  /// path is needed, and it will be scheduled in a normal context.
  ///
  /// @param interrupt_number The interrupt that was fired.
  ///
  /// @return true if the system should execute the slow path part of this receiver, and false if the driver has
  ///         completely handled whatever caused this interrupt to fire (or if the interrupt was not related to the
  ///         device handled by this driver)
  virtual bool handle_interrupt_fast(uint8_t interrupt_number) = 0;

  /// @brief The second pass of handling an interrupt. This is called in a normal kernel thread context, not within the
  ///        interrupt handler itself.
  ///
  /// The system received an interrupt that this receiver was registered for, and the receiver requested the slow-path
  /// be executed by returning `false` to `handle_interrupt_fast()`. The receiver should now do whatever processing it
  /// needs to do. It may request locks as needed.
  ///
  /// @param interrupt_number The IRQ that was fired.
  virtual void handle_interrupt_slow(uint8_t interrupt_number) { };
};

#ifdef AZALEA_TEST_CODE
void test_only_reset_name_counts();
#endif

#endif
