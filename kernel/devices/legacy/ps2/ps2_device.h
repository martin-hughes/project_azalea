/// @file
/// @brief PS/2 device drivers header. Provides generic PS/2 device drivers that can be extended as needed by anything
/// more fancy

#ifndef DEVICE_PS2_DEVICE_HDR
#define DEVICE_PS2_DEVICE_HDR

#include "devices/device_interface.h"
#include "user_interfaces/error_codes.h"
#include "klib/data_structures/string.h"
#include "devices/generic/gen_keyboard.h"
#include "devices/generic/gen_mouse.h"
#include <memory>

class gen_ps2_controller_device;

/// @brief A generic PS/2 device.
///
/// This driver does nothing, to actually communicate with a PS/2 device it is necessary to write a derived class.
class gen_ps2_device : public IDevice, IInterruptReceiver
{
public:
  gen_ps2_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel);
  gen_ps2_device(std::shared_ptr<gen_ps2_controller_device> parent,
                 bool second_channel,
                 const kl_string human_name,
                 const kl_string dev_name);
  virtual ~gen_ps2_device();

  // IIrqReceiver interface
  virtual bool handle_interrupt_fast(uint8_t irq_number) override;
  virtual void handle_interrupt_slow(uint8_t irq_number) override;

  // Overrides of IDevice
  bool start() override;
  bool stop() override;
  bool reset() override;

protected:
  void enable_irq();
  void disable_irq();

  std::weak_ptr<gen_ps2_controller_device> _parent; ///< The parent controller device
  bool _second_channel; ///< Is this on the second channel of the controller?
  bool _irq_enabled; ///< Is the IRQ enabled?
};

/// @brief Driver for a generic PS/2 mouse.
///
class ps2_mouse_device : public gen_ps2_device, public generic_mouse
{
public:
  ps2_mouse_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel);
};

/// @brief Driver for a generic PS/2 keyboard.
///
class ps2_keyboard_device : public gen_ps2_device, public generic_keyboard
{
public:
  ps2_keyboard_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel);

  // IIrqReceiver interface.
  virtual bool handle_interrupt_fast(uint8_t irq_number) override;
  virtual void handle_interrupt_slow(uint8_t irq_number) override;

  // Overrides of IDevice
  bool start() override;

protected:
  special_keys _spec_keys_down; ///< Which special keys are currently pressed?
  bool _next_key_is_release; ///< Is the next input telling us about a key that has been released?
  bool _next_key_is_special; ///< Is the next input for a special key?
  unsigned int _pause_seq_chars; ///< How many characters of the 'pause' key have been transmitted?
};

#endif