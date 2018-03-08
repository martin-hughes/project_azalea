/// @file Generic device driver interfaces.
///
/// All device drivers must inherit from IDevice, other generic but widely used behaviours have their own interfaces.

#ifndef CORE_DEVICE_INTFACE_HEADER
#define CORE_DEVICE_INTFACE_HEADER

#include "klib/data_structures/string.h"

enum class DEV_STATUS
{
  OK,
  FAILED,
  STOPPED,
  NOT_PRESENT,
};

/// @brief The interface that all device drivers must inherit from.
///
/// This doesn't do much at the moment apart from indicating that the child class is a device driver, but expect it to
/// expand in future.
class IDevice
{
public:
  virtual ~IDevice() = default;

  virtual const kl_string device_name() = 0;
  virtual DEV_STATUS get_device_status() = 0;
};

/// @brief An interface that must be inherited by all drivers that handle IRQs.
///
/// More than one device may opt to handle the same IRQ.
class IIrqReceiver
{
public:
  virtual ~IIrqReceiver() = default;

  /// @brief Handle an IRQ.
  ///
  /// The system has received an IRQ and determined that this object was registered as a handler. Since each object
  /// could, in principle, be registered for more than one IRQ the system passes the IRQ number to the handler. No
  /// other data is passed since the system doesn't know what data it should be passing!
  ///
  /// Since the system is running inside an IRQ handler care should be taken not to run for too long, since this
  /// processor cannot execute another task until the whole IRQ handler is complete.
  ///
  /// @param irq_number The IRQ that was fired.
  ///
  /// @return true if the system should continue executing other IRQ handlers for this request. false if this handler
  ///         has definitely handled the IRQ and no other devices need to be considered. False should be used with
  ///         *extreme* caution.
  virtual bool handle_irq(unsigned char irq_number) = 0;
};

#endif
