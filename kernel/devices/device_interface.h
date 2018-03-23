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
/// The receiver is split in to two parts - fast and slow paths. The fast path is always executed by the global IRQ
/// handler, and runs as part of the IRQ handling code. Interrupts are disabled while it runs. The slow path runs as
/// part of a normal kernel thread, and is optional - it is called if the fast path requests it to be.
///
/// The fast path cannot call any part of the kernel that locks - which is quite a lot of it! The slow path can call
/// anything it needs to.
///
/// More than one device may opt to handle the same IRQ.
class IIrqReceiver
{
public:
  virtual ~IIrqReceiver() = default;

  /// @brief The first pass of handling an IRQ. This is called while the interrupt itself is still being handled.
  ///
  /// The system has received an IRQ and determined that this object was registered as a handler. Since each object
  /// could, in principle, be registered for more than one IRQ the system passes the IRQ number to the handler. No
  /// other data is passed since the system doesn't know what data it should be passing!
  ///
  /// Since the system is running inside an IRQ handler care should be taken not to run for too long, since this
  /// processor cannot execute another task until the whole IRQ handler is complete.
  ///
  /// Additionally, this function *MUST NOT* lock at all, since it may be called while the lock is owned by a normal,
  /// non-interrupt thread. In that case, the IRQ handler would never return, since the IRQ would be unable to gain the
  /// lock and the non-interrupt thread unable to release it. Note that some common functions DO lock - e.g.
  /// `new`/`kmalloc` or `msg_send_to_process()`.
  ///
  /// Should the owner of this receiver need to do anything involving a lock, return true to indicate that the slow
  /// path is needed, and it will be scheduled in a normal context.
  ///
  /// @param irq_number The IRQ that was fired.
  ///
  /// @return true if the system should execute the slow path part of this receiver, and false if the driver has
  ///         completely handled whatever caused this IRQ to fire (or if the IRQ was not related to the device handled
  ///         by this driver)
  virtual bool handle_irq_fast(unsigned char irq_number) = 0;

  /// @brief The second pass of handling an IRQ. This is called in a normal kernel thread context, not within the
  ///        interrupt handler itself.
  ///
  /// The system received an interrupt that this receiver was registered for, and the receiver requested the slow-path
  /// be executed by returning `false` to `handle_irq_fast()`. The receiver should now do whatever processing it needs
  /// to do. It may request locks as needed.
  ///
  /// @param irq_number The IRQ that was fired.
  virtual void handle_irq_slow(unsigned char irq_number) { };
};

#endif
