/// @file PS/2 device drivers header. Provides generic PS/2 device drivers that can be extended as needed by anything
/// more fancy

#ifndef DEVICE_PS2_DEVICE_HDR
#define DEVICE_PS2_DEVICE_HDR

#include "devices/device_interface.h"
#include "klib/misc/error_codes.h"
#include "klib/data_structures/string.h"

class gen_ps2_controller_device;
class task_process;

class gen_ps2_device : public IDevice, IIrqReceiver
{
public:
  gen_ps2_device(gen_ps2_controller_device *parent, bool second_channel);
  virtual ~gen_ps2_device();

  // Core IDevice interface
  virtual const kl_string device_name();
  virtual DEV_STATUS get_device_status();

  // IIrqReceiver interface
  virtual bool handle_irq_fast(unsigned char irq_number);
  virtual void handle_irq_slow(unsigned char irq_number);

protected:
  void enable_irq();
  void disable_irq();

  kl_string _device_name;
  DEV_STATUS _status;
  gen_ps2_controller_device *_parent;
  bool _second_channel;
  bool _irq_enabled;
};

class ps2_mouse_device : public gen_ps2_device
{
public:
  ps2_mouse_device(gen_ps2_controller_device *parent, bool second_channel);
};

class ps2_keyboard_device : public gen_ps2_device
{
public:
  ps2_keyboard_device(gen_ps2_controller_device *parent, bool second_channel);

  // IIrqReceiver interface.
  virtual bool handle_irq_fast(unsigned char irq_number);
  virtual void handle_irq_slow(unsigned char irq_number);

  // Process that should receive key press messages. This is only intended to be temporary, until the driver structure
  // gets a bit more flesh in it.
  task_process *recipient;
};

#endif