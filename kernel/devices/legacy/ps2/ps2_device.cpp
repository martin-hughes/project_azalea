/// @file Generic implementation of the two main PS/2 device types - mouse and keyboard.
///
/// Many functions in this file have no particular documentation, since the documentation would be the same as the
/// interface class they derive from.
// Known defects:
// - Keyboard currently only deals with scancode set 2
// - If the device fails, we just get stuck.
// - The switch (scancode) block could probably be folded in to the translated scancode part.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "devices/generic/gen_keyboard.h"
#include "devices/legacy/ps2/ps2_device.h"
#include "devices/legacy/ps2/ps2_controller.h"

extern const KEYS ps2_set_2_norm_scancode_map[256];
extern const KEYS ps2_set_2_spec_scancode_map[256];
extern const uint32_t ps2_set_2_props_tab_len;
extern key_props ps2_set_2_key_props[];

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

bool gen_ps2_device::handle_irq_fast(uint8_t irq_number)
{
  return false;
}

void gen_ps2_device::handle_irq_slow(uint8_t irq_number)
{

}

/// @brief Enable the sending of IRQs to this device.
///
/// The device will register for the correct IRQ for the channel it is on and update the parent controller device's
/// config to enable those IRQs.
void gen_ps2_device::enable_irq()
{
  KL_TRC_ENTRY;

  uint8_t irq_num;

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

  uint8_t irq_num;

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
  recipient(nullptr),
  _next_key_is_release(false),
  _next_key_is_special(false),
  _spec_keys_down({ false, false, false, false, false, false, false }),
  _pause_seq_chars(0)
{
  KL_TRC_ENTRY;

  _device_name = "Generic PS/2 keyboard";
  _status = DEV_STATUS::OK;

  this->enable_irq();

  KL_TRC_EXIT;
}

bool ps2_keyboard_device::handle_irq_fast(uint8_t irq_num)
{
  // Simply do all of our handling in the slow path part of the handler.
  return true;
}

void ps2_keyboard_device::handle_irq_slow(uint8_t irq_num)
{
  KL_TRC_ENTRY;
  uint8_t scancode;
  KEYS translated_code;
  task_process *proc = this->recipient;
  klib_message_hdr hdr;
  char printable_char;
  keypress_msg *updown_msg;
  key_char_msg *char_msg;


  hdr.msg_length = sizeof(keypress_msg);
  hdr.originating_process = task_get_cur_thread()->parent_process;

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

        case 0xE1:
          this->_pause_seq_chars = 1;

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

          printable_char = keyb_translate_key(translated_code,
                                              this->_spec_keys_down,
                                              ps2_set_2_key_props,
                                              ps2_set_2_props_tab_len);

          this->_next_key_is_special = false;


          if (proc != nullptr)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Send keypress to recipient... \n");
            updown_msg = new keypress_msg;
            updown_msg->key_pressed = translated_code;
            updown_msg->modifiers = this->_spec_keys_down;
            hdr.msg_contents = reinterpret_cast<char *>(updown_msg);
            hdr.msg_id = (this->_next_key_is_release ? SM_KEYUP : SM_KEYDOWN);
            hdr.msg_length = sizeof(keypress_msg);

            msg_send_to_process(proc, hdr);

            if ((!this->_next_key_is_release) && (printable_char != 0))
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Send character message\n");
              char_msg = new key_char_msg;
              char_msg->pressed_character = printable_char;

              hdr.msg_contents = reinterpret_cast<char *>(char_msg);
              hdr.msg_id = SM_PCHAR;
              hdr.msg_length = sizeof(key_char_msg);

              msg_send_to_process(proc, hdr);
            }
          }
        }
        this->_next_key_is_release = false;
      }
    }
  }

  KL_TRC_EXIT;
}