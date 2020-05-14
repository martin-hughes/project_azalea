/// @file
/// @brief Generic implementation of the two main PS/2 device types - mouse and keyboard.
///
/// Many functions in this file have no particular documentation, since the documentation would be the same as the
/// interface class they derive from.
// Known defects:
// - Keyboard currently only deals with scancode set 2
// - If the device fails, we just get stuck.
// - The switch (scancode) block could probably be folded in to the translated scancode part.
// - Start/stop/reset (etc) are not properly supported.

//#define ENABLE_TRACING

#include <string>

#include "klib/klib.h"
#include "processor/processor.h"
#include "devices/generic/gen_keyboard.h"
#include "devices/legacy/ps2/ps2_device.h"
#include "devices/legacy/ps2/ps2_controller.h"

extern const KEYS ps2_set_2_norm_scancode_map[256]; ///< Normal PS/2 scancode map.
extern const KEYS ps2_set_2_spec_scancode_map[256]; ///< Scancode for 'special' keys on a PS/2 keyboard.

/// @brief Standard constructor
///
/// @param parent The parent PS/2 controller device.
///
/// @param second_channel Is this device connected to the second channel of the controller?
///
/// @param human_name The name to associate with this device.
///
/// @param dev_name The device name to associate with this device.
gen_ps2_device::gen_ps2_device(std::shared_ptr<gen_ps2_controller_device> parent,
                               bool second_channel,
                               const std::string human_name,
                               const std::string dev_name) :
  IDevice{human_name, dev_name, true},
  _parent{parent},
  _second_channel{second_channel},
  _irq_enabled{false}
{
  KL_TRC_ENTRY;
  ASSERT(parent);
  KL_TRC_EXIT;
}

/// @brief Constructor for devices with no particular name.
///
/// @param parent The parent PS/2 controller device.
///
/// @param second_channel Is this device connected to the second channel of the controller?
gen_ps2_device::gen_ps2_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel) :
  gen_ps2_device{parent, second_channel, "Generic PS/2 device", "ps2d"}
{ }

gen_ps2_device::~gen_ps2_device()
{

}

/// @brief Override of IDevice::start() - enables the device.
///
/// @return True if this was a valid call to start. False otherwise.
bool gen_ps2_device::start()
{
  set_device_status(DEV_STATUS::OK);
  return true;
}

/// @brief Override of IDevice::start() - enables the device.
///
/// @return True if this was a valid call to start. False otherwise.
bool gen_ps2_device::stop()
{
  set_device_status(DEV_STATUS::STOPPED);
  return true;
}

/// @brief Override of IDevice::start() - enables the device.
///
/// @return True if this was a valid call to start. False otherwise.
bool gen_ps2_device::reset()
{
  set_device_status(DEV_STATUS::STOPPED);
  return true;
}

bool gen_ps2_device::handle_interrupt_fast(uint8_t irq_number)
{
  return false;
}

void gen_ps2_device::handle_interrupt_slow(uint8_t irq_number)
{

}

/// @brief Enable the sending of IRQs to this device.
///
/// The device will register for the correct IRQ for the channel it is on and update the parent controller device's
/// config to enable those IRQs.
void gen_ps2_device::enable_irq()
{
  uint8_t irq_num;
  std::shared_ptr<gen_ps2_controller_device> parent = _parent.lock();

  KL_TRC_ENTRY;

  if (parent)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent still exists\n");

    ASSERT(!this->_irq_enabled);

    irq_num = this->_second_channel ? 12 : 1;
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Registering for IRQ: ", irq_num, "\n");

    proc_register_irq_handler(irq_num, dynamic_cast<IInterruptReceiver *>(this));

    gen_ps2_controller_device::ps2_config_register reg;
    reg = parent->read_config();

    if (_second_channel)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Enabling second channel IRQ\n");
      reg.flags.second_port_interrupt_enabled = 1;
    }
    else
    {
      reg.flags.first_port_interrupt_enabled = 1;
    }

    parent->write_config(reg);
    _irq_enabled = true;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent device destroyed\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  KL_TRC_EXIT;
}

/// @brief Disable the sending of IRQs to this device.
///
/// The device will unregister itself for IRQ handling and update the parent controller's config.
void gen_ps2_device::disable_irq()
{
  uint8_t irq_num;
  std::shared_ptr<gen_ps2_controller_device> parent = _parent.lock();

  KL_TRC_ENTRY;

  if (parent)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent still exists\n");

    ASSERT(this->_irq_enabled);

    irq_num = this->_second_channel ? 12 : 1;
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Unregistering IRQ: ", irq_num, "\n");

    proc_unregister_irq_handler(irq_num, dynamic_cast<IInterruptReceiver *>(this));

    gen_ps2_controller_device::ps2_config_register reg;
    reg = parent->read_config();

    if (_second_channel)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Disabling second channel IRQ\n");
      reg.flags.second_port_interrupt_enabled = 0;
    }
    else
    {
      reg.flags.first_port_interrupt_enabled = 0;
    }

    parent->write_config(reg);
    _irq_enabled = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent device destroyed\n");
    set_device_status(DEV_STATUS::FAILED);
  }

  KL_TRC_EXIT;
}

/// @brief Standard constructor
///
/// @param parent The parent PS/2 controller device.
///
/// @param second_channel Is this device connected to the second channel of the controller?
ps2_mouse_device::ps2_mouse_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel) :
  gen_ps2_device{parent, second_channel, "Generic PS/2 mouse", "ps2m"}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Standard constructor
///
/// @param parent The parent PS/2 controller device.
///
/// @param second_channel Is this device connected to the second channel of the controller?
ps2_keyboard_device::ps2_keyboard_device(std::shared_ptr<gen_ps2_controller_device> parent, bool second_channel) :
  gen_ps2_device{parent, second_channel, "Generic PS/2 keyboard", "ps2k"},
  _next_key_is_release{false},
  _next_key_is_special{false},
  _pause_seq_chars{0}
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

bool ps2_keyboard_device::start()
{
  uint8_t response;
  std::shared_ptr<gen_ps2_controller_device> parent = _parent.lock();
  bool result{true};

  KL_TRC_ENTRY;

  set_device_status(DEV_STATUS::STARTING);

  if (parent)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent running\n");
    parent->send_byte(PS2_CONST::DEV_ENABLE_SCANNING, this->_second_channel);
    parent->read_byte(response);

    this->enable_irq();
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Parent not available\n");
    result = false;
    set_device_status(DEV_STATUS::FAILED);
  }

  if (result)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Successful start\n");
    set_device_status(DEV_STATUS::OK);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

bool ps2_keyboard_device::handle_interrupt_fast(uint8_t irq_num)
{
  // Simply do all of our handling in the slow path part of the handler.
  KL_TRC_TRACE(TRC_LVL::FLOW, "Fast interrupt\n");
  return true;
}

void ps2_keyboard_device::handle_interrupt_slow(uint8_t irq_num)
{
  KL_TRC_ENTRY;
  uint8_t scancode;
  KEYS translated_code;

  while ((proc_read_port(0x64, 8) & 0x1) == 1)
  {
    scancode = static_cast<uint8_t>(proc_read_port(0x60, 8));
    KL_TRC_TRACE(TRC_LVL::FLOW, "Keyboard data: ", scancode, "\n");

    if (this->_pause_seq_chars != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Still mid pause seq.\n");
      this->_pause_seq_chars++;

      // The entire pause character sequence is 8 bytes long
      if (this->_pause_seq_chars == 8)
      {
        this->_pause_seq_chars = 0;
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Scancode: ", scancode, "\n");

      // Scancode conversion table. For the time being, only deal with Scancode set 2.
      switch (scancode)
      {
        case 0x00:
        case 0xAA:
        case 0xEE:
        case 0xFA:
        case 0xFC:
        case 0xFD:
        case 0xFE:
        case 0xFF:
          // special command responses, these can be ignored for now.
          break;

        case 0xF0:
          // Next scan code is for a key that has been released
          this->_next_key_is_release = true;
          break;

        case 0x11:
          // Left or right Alt
          if (this->_next_key_is_special)
          {
            this->_spec_keys_down.right_alt = !this->_next_key_is_release;
          }
          else
          {
            this->_spec_keys_down.left_alt = !this->_next_key_is_release;
          }
          break;

        case 0x12:
          if (this->_next_key_is_special)
          {
            // Beginning or end of print screen sequence.
            this->_spec_keys_down.print_screen_start = !this->_next_key_is_release;
          }
          else
          {
            // Left shift
            this->_spec_keys_down.left_shift = !this->_next_key_is_release;
          }
          break;

        case 0x14:
          // Left or right Control
          if (this->_next_key_is_special)
          {
            this->_spec_keys_down.right_control = !this->_next_key_is_release;
          }
          else
          {
            this->_spec_keys_down.left_control = !this->_next_key_is_release;
          }
          break;

        case 0x59:
          // Right shift
          this->_spec_keys_down.right_shift = !this->_next_key_is_release;
          break;

        case 0xE0:
          // Next scan code is for special key
          this->_next_key_is_special = true;
          break;

        case 0xE1:
          this->_pause_seq_chars = 1;
          break;

        default:
          // All other key presses.
          break;
      }

      // If we've reached a non-special character scan code then the next scan code will be a normal one. The "special
      // key" 0xE0 scancode should come before the key release scancode 0xF0 for the relevant special key. E.g. Right
      // Ctrl release is 0xE0 0xF0 0x14.
      if ((scancode != 0xF0) && (scancode != 0xE1))
      {
        if (scancode != 0xE0)
        {
          if (this->_next_key_is_special)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Translating special key\n");
            translated_code = ps2_set_2_spec_scancode_map[scancode];
          }
          else
          {
            translated_code = ps2_set_2_norm_scancode_map[scancode];
          }

          KL_TRC_TRACE(TRC_LVL::FLOW, "Translated: ", (int)translated_code, "\n");

          if (this->_next_key_is_release)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Post release message\n");
            this->handle_key_up(translated_code, this->_spec_keys_down);
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Post press message\n");
            this->handle_key_down(translated_code, this->_spec_keys_down);
          }

          this->_next_key_is_special = false;
        }
        this->_next_key_is_release = false;
      }
    }
  }

  KL_TRC_EXIT;
}
