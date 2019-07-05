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

class gen_ps2_controller_device;

/// @brief A generic PS/2 device.
///
/// This driver does nothing, to actually communicate with a PS/2 device it is necessary to write a derived class.
class gen_ps2_device : public IDevice, IInterruptReceiver
{
public:
  gen_ps2_device(gen_ps2_controller_device *parent, bool second_channel);
  gen_ps2_device(gen_ps2_controller_device *parent, bool second_channel, const kl_string name);
  virtual ~gen_ps2_device();

  // IIrqReceiver interface
  virtual bool handle_interrupt_fast(uint8_t irq_number) override;
  virtual void handle_interrupt_slow(uint8_t irq_number) override;

protected:
  void enable_irq();
  void disable_irq();

  gen_ps2_controller_device *_parent;
  bool _second_channel;
  bool _irq_enabled;
};

/// @brief Driver for a generic PS/2 mouse.
///
class ps2_mouse_device : public gen_ps2_device, public generic_mouse
{
public:
  ps2_mouse_device(gen_ps2_controller_device *parent, bool second_channel);
};

/// @brief Driver for a generic PS/2 keyboard.
///
class ps2_keyboard_device : public gen_ps2_device, public generic_keyboard
{
public:
  ps2_keyboard_device(gen_ps2_controller_device *parent, bool second_channel);

  // IIrqReceiver interface.
  virtual bool handle_interrupt_fast(uint8_t irq_number) override;
  virtual void handle_interrupt_slow(uint8_t irq_number) override;

protected:
  special_keys _spec_keys_down;
  bool _next_key_is_release;
  bool _next_key_is_special;
  unsigned int _pause_seq_chars;
};

#endif