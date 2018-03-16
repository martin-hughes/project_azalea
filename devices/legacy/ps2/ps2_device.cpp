/// @file Generic implementation of the two main PS/2 device types - mouse and keyboard.
///
/// Many functions in this file have no particular documentation, since the documentation would be the same as the
/// interface class they derive from.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "devices/legacy/ps2/ps2_device.h"
#include "devices/legacy/ps2/ps2_controller.h"

gen_ps2_device::gen_ps2_device(gen_ps2_controller_device *parent, bool second_channel) :
  _device_name("Generic PS/2 device"),
  _status(DEV_STATUS::FAILED),
  _parent(parent),
  _second_channel(second_channel),
  _irq_enabled(false)
{
  KL_TRC_ENTRY;
  ASSERT(_parent != nullptr);
  KL_TRC_EXIT;
}

gen_ps2_device::~gen_ps2_device()
{

}

const kl_string gen_ps2_device::device_name()
{
  return _device_name;
}

DEV_STATUS gen_ps2_device::get_device_status()
{
  return _status;
}

bool gen_ps2_device::handle_irq_fast(unsigned char irq_number)
{
  return false;
}

void gen_ps2_device::handle_irq_slow(unsigned char irq_number)
{

}

/// @brief Enable the sending of IRQs to this device.
///
/// The device will register for the correct IRQ for the channel it is on and update the parent controller device's
/// config to enable those IRQs.
void gen_ps2_device::enable_irq()
{
  KL_TRC_ENTRY;

  unsigned char irq_num;

  ASSERT(!this->_irq_enabled);

  irq_num = this->_second_channel ? 12 : 1;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Registering for IRQ: ", irq_num, "\n");

  proc_register_irq_handler(irq_num, dynamic_cast<IIrqReceiver *>(this));

  gen_ps2_controller_device::ps2_config_register reg;
  reg = _parent->read_config();

  if (_second_channel)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Enabling second channel IRQ\n");
    reg.flags.second_port_interrupt_enabled = 1;
  }
  else
  {
    reg.flags.first_port_interrupt_enabled = 1;
  }

  _parent->write_config(reg);
  _irq_enabled = true;

  KL_TRC_EXIT;
}

/// @brief Disable the sending of IRQs to this device.
///
/// The device will unregister itself for IRQ handling and update the parent controller's config.
void gen_ps2_device::disable_irq()
{
  KL_TRC_ENTRY;

  unsigned char irq_num;

  ASSERT(this->_irq_enabled);

  irq_num = this->_second_channel ? 12 : 1;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Unregistering IRQ: ", irq_num, "\n");

  proc_unregister_irq_handler(irq_num, dynamic_cast<IIrqReceiver *>(this));

  gen_ps2_controller_device::ps2_config_register reg;
  reg = _parent->read_config();

  if (_second_channel)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Disabling second channel IRQ\n");
    reg.flags.second_port_interrupt_enabled = 0;
  }
  else
  {
    reg.flags.first_port_interrupt_enabled = 0;
  }

  _parent->write_config(reg);
  _irq_enabled = false;

  KL_TRC_EXIT;
}

ps2_mouse_device::ps2_mouse_device(gen_ps2_controller_device *parent, bool second_channel) :
  gen_ps2_device(parent, second_channel)
{
  KL_TRC_ENTRY;

  _device_name = "Generic PS/2 mouse";
  _status = DEV_STATUS::OK;

  KL_TRC_EXIT;
}

ps2_keyboard_device::ps2_keyboard_device(gen_ps2_controller_device *parent, bool second_channel) :
  gen_ps2_device(parent, second_channel),
  recipient(nullptr)
{
  KL_TRC_ENTRY;

  _device_name = "Generic PS/2 keyboard";
  _status = DEV_STATUS::OK;

  this->enable_irq();

  KL_TRC_EXIT;
}

bool ps2_keyboard_device::handle_irq_fast(unsigned char irq_num)
{
  // Simply do all of our handling in the slow path part of the handler.
  return true;
}

void ps2_keyboard_device::handle_irq_slow(unsigned char irq_num)
{
  KL_TRC_ENTRY;
  unsigned char response;
  task_process *proc = this->recipient;
  klib_message_hdr hdr;

  hdr.msg_id = 1;
  hdr.msg_length = 1;
  hdr.originating_process = task_get_cur_thread()->parent_process;

  char *msg;

  while ((proc_read_port(0x64, 8) & 0x1) == 1)
  {
    response = static_cast<unsigned char>(proc_read_port(0x60, 8));
    KL_TRC_TRACE(TRC_LVL::FLOW, "Keyboard data: ", response, "\n");

    if (proc != nullptr)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Dump scan code to recipient... \n");
      msg = new char[1];
      *msg = response;
      hdr.msg_contents = msg;

      msg_send_to_process(proc, hdr);
    }
  }

  KL_TRC_EXIT;
}