/// @file
/// @brief A simple PS/2 controller driver. PS/2 connected devices are dealt with separately.

// Known issues:
// - During PS/2 device startup, only one attempt is made to reset devices before they are declared failed.
// - Failure of the device on channel 1 causes the controller to be considered failed, inhibiting device 2.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "ps2_controller.h"

/// @brief Standard constructor.
gen_ps2_controller_device::gen_ps2_controller_device() :
  IDevice{"Generic PS/2 controller"},
  chan_1_dev(nullptr),
  chan_2_dev(nullptr)
{
  KL_TRC_ENTRY;

  // Unless anything goes wrong, assume the PS/2 controller is fine.
  current_dev_status = DEV_STATUS::OK;

  // Start by assuming a two-channel PS/2 controller. We check this assumption below.
  _dual_channel = true;

  // For the time being, claim no connected devices.
  _chan_1_dev_type = PS2_DEV_TYPE::NONE_CONNECTED;
  _chan_2_dev_type = PS2_DEV_TYPE::NONE_CONNECTED;

  // Accomplish the following steps (thanks 'https://wiki.osdev.org/%228042%22_PS/2_Controller' for the pointers!)
  // 1 - Disable both connected devices.
  // 2 - Flush the controller output buffer
  // 3 - Set the controller configuration
  // 4 - Self-test the controller
  // 5 - Determine whether this is a one-channel or two-channel controller
  // 6 - Perform interface tests
  // 7 - Enable the devices
  // 8 - Reset them
  // 9 - And finally, identify the devices.
  // Both devices are enabled, but with scanning and IRQs turned off.

  uint8_t response;

  // 1 - Disable both connected devices.
  send_ps2_command(PS2_CONST::DISABLE_DEV_1, false, 0, false, response);
  send_ps2_command(PS2_CONST::DISABLE_DEV_2, false, 0, false, response);

  // 2 - Flush the controller output buffer. Do this by reading the data port until the buffer is reported empty.
  while (this->read_status().flags.output_buffer_status != 0)
  {
    proc_read_port(PS2_DATA_PORT, 8);
  }

  // 3 - Set the controller configuration
  ps2_config_register config;
  send_ps2_command(PS2_CONST::READ_CONFIG, false, 0, true, config.raw);
  config.flags.first_port_interrupt_enabled = 0;
  config.flags.second_port_interrupt_enabled = 0;
  config.flags.first_port_translation = 0;
  send_ps2_command(PS2_CONST::WRITE_CONFIG, true, config.raw, false, response);

  // 4 - Self-test the controller. If it fails, don't continue.
  send_ps2_command(PS2_CONST::SELF_TEST, false, 0, true, response);
  if (response != PS2_CONST::SELF_TEST_SUCCESS)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "PS/2 self-test failed\n");
    current_dev_status = DEV_STATUS::FAILED;
  }
  else
  {
    // 5 - Determine the number of channels. Do this by looking to see whether the clock disable state is as expected.
    // When the device is disabled, the byte should be equal to 1.
    if (config.flags.second_port_clock_disable == 0)
    {
      _dual_channel = false;
    }
    else
    {
      // No point doing these tests if we already know the controller is single-channel.
      send_ps2_command(PS2_CONST::ENABLE_DEV_2, false, 0, false, response);
      send_ps2_command(PS2_CONST::READ_CONFIG, false, 0, true, config.raw);
      if (config.flags.second_port_clock_disable == 1)
      {
        _dual_channel = false;
      }
      send_ps2_command(PS2_CONST::DISABLE_DEV_2, false, 0, false, response);
    }

    // 6 - Perform interface tests
    send_ps2_command(PS2_CONST::DEV_1_PORT_TEST, false, 0, true, response);
    if (response == PS2_CONST::PORT_TEST_SUCCESS)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Test of channel 1 successful\n");

      send_ps2_command(PS2_CONST::DEV_2_PORT_TEST, false, 0, true, response);
      if (response != PS2_CONST::PORT_TEST_SUCCESS)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Test of channel 2 failed, revert to single channel\n");
        _dual_channel = false;
      }

      // 7 & 8 - Enable and reset the devices - device 1.
      send_ps2_command(PS2_CONST::ENABLE_DEV_1, false, 0, false, response);
      send_byte(PS2_CONST::DEV_RESET, false);
      read_byte(response);

      if (response == PS2_CONST::DEV_SELF_TEST_OK)
      {
        read_byte(response);
      }
      if (response != PS2_CONST::DEV_CMD_ACK)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Device one failed to reset\n");
        current_dev_status = DEV_STATUS::FAILED;
      }

      send_byte(PS2_CONST::DEV_DISABLE_SCANNING, false);
      read_byte(response);

      // 7 & 8 - device 2.
      if (_dual_channel)
      {
        send_ps2_command(PS2_CONST::ENABLE_DEV_2, false, 0, false, response);

        send_byte(PS2_CONST::DEV_RESET, true);
        read_byte(response);

        if (response == PS2_CONST::DEV_SELF_TEST_OK)
        {
          read_byte(response);
        }
        if (response != PS2_CONST::DEV_CMD_ACK)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Device one failed to reset\n");
          current_dev_status = DEV_STATUS::FAILED;
        }

        send_byte(PS2_CONST::DEV_DISABLE_SCANNING, true);
        read_byte(response);
      }

      // 9 - Identify the devices.
      _chan_1_dev_type = this->identify_device(false);
      if (_dual_channel)
      {
        _chan_2_dev_type = this->identify_device(true);
      }

      // Construct and enable child devices.
      chan_1_dev = this->instantiate_device(false, _chan_1_dev_type);
      if (_dual_channel)
      {
        chan_2_dev = this->instantiate_device(true, _chan_2_dev_type);
      }
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Test of channel 1 failed, response: ", response, "\n");
      current_dev_status = DEV_STATUS::FAILED;
    }
  }

  KL_TRC_EXIT;
}

gen_ps2_controller_device::~gen_ps2_controller_device()
{
  KL_TRC_ENTRY;

  KL_TRC_EXIT;
}

/// @brief Send a command to the PS/2 controller.
///
/// Optionally, send a piece of follow-up data, and/or wait for a response. At present, this code has no way of
/// detecting a failure, so if a command is meant to respond and doesn't the code will deadlock.
///
/// @param[in] command The command to send
///
/// @param[in] needs_second_byte Set to true if this command requires a second byte to be sent as well.
///
/// @param[in] second_byte If needs_second_byte is set to true, this byte is sent to the controller as well. Otherwise,
///                        it is ignored.
///
/// @param[in] expect_response Set to true if the command sent generates a response code, in which case that code is
///                            copied in to response.
///
/// @param[out] response If expect_response is set to true, this parameter is set to the response received from the
///                      PS/2 controller. If expect_response is false, then response is left unchanged.
///
/// @return ERR_CODE::NO_ERROR in all cases. In the future, when deadlock detection is implemented, other codes will be
///         returned.
ERR_CODE gen_ps2_controller_device::send_ps2_command(uint8_t command,
                                                     bool needs_second_byte,
                                                     uint8_t second_byte,
                                                     bool expect_response,
                                                     uint8_t &response)
{
  KL_TRC_ENTRY;

  proc_write_port(PS2_COMMAND_PORT, command, 8);

  if (needs_second_byte)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Send second byte\n");

    while ((proc_read_port(PS2_COMMAND_PORT, 8) & 0x2) != 0)
    {
    }

    KL_TRC_TRACE(TRC_LVL::FLOW, "Sending\n");
    proc_write_port(PS2_DATA_PORT, second_byte, 8);
  }

  if (expect_response)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Waiting for response\n");
    while ((proc_read_port(PS2_COMMAND_PORT, 8) & 0x1) == 0)
    {
    }

    KL_TRC_TRACE(TRC_LVL::FLOW, "Reading\n");
    response = static_cast<uint8_t>(proc_read_port(PS2_DATA_PORT, 8));
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Response: ", response, "\n");
  }

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

/// @brief Retrieve the PS/2 controller's status register
///
/// @return The contents of the controller's status register.
gen_ps2_controller_device::ps2_status_register gen_ps2_controller_device::read_status()
{
  KL_TRC_ENTRY;

  ps2_status_register reg;
  reg.raw = proc_read_port(PS2_COMMAND_PORT, 8);

  KL_TRC_EXIT;

  return reg;
}

/// @brief Send a byte to the connected PS/2 device.
///
/// @param data The data byte to send
///
/// @param second_channel If set to true, send this data to device 2. Otherwise to device 1. If set to true and the
///                       controller is single channel, the function fails.
///
/// @return ERR_CODE::NO_ERROR if the byte is sent successfully, ERR_CODE::INVALID_OP if the byte was meant for the
///         second device but this is a single channel controller.
ERR_CODE gen_ps2_controller_device::send_byte(uint8_t data, bool second_channel)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  if (second_channel)
  {
    if (!_dual_channel)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Requested second channel on single channel device\n");
      res = ERR_CODE::INVALID_OP;
    }
    else
    {
      proc_write_port(PS2_COMMAND_PORT, PS2_CONST::DEV_2_NEXT, 8);
    }
  }

  if (res == ERR_CODE::NO_ERROR)
  {
    proc_write_port(PS2_DATA_PORT, data, 8);
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returning: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Read a byte from the connected PS/2 device.
///
/// If no data is waiting, wait until some is.
///
/// Note that this function makes no attempt to determine whether that data came from the first or second device.
///
/// @param[out] data The next byte of retrieved data.
///
/// @return ERR_CODE::NO_ERROR. Other codes are possible in future.
ERR_CODE gen_ps2_controller_device::read_byte(uint8_t &data)
{
  KL_TRC_ENTRY;

  ERR_CODE res = ERR_CODE::NO_ERROR;

  while(read_status().flags.output_buffer_status == 0)
  { }

  data = proc_read_port(PS2_DATA_PORT, 8);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Data: ", data, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Returning: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Identify the type of device connected to the PS/2 controller.
///
/// @param second_channel If true, send the command to the second channel of the controller.
///
/// @return The type of device connected.
PS2_DEV_TYPE gen_ps2_controller_device::identify_device(bool second_channel)
{
  KL_TRC_ENTRY;

  PS2_DEV_TYPE dev_type = PS2_DEV_TYPE::NONE_CONNECTED;
  uint8_t response;

  send_byte(PS2_CONST::DEV_IDENTIFY, second_channel);
  read_byte(response);

  if (response == PS2_CONST::DEV_CMD_ACK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Identify command successful\n");
    while (response == PS2_CONST::DEV_CMD_ACK)
    {
      read_byte(response);
    }

    KL_TRC_TRACE(TRC_LVL::EXTRA, "Identification response:", response, "\n");

    // The numbers contained within this switch statement are so specific it's not worth forming a whole enum for them.
    switch (response)
    {
      case 0x00:
        KL_TRC_TRACE(TRC_LVL::FLOW, "ID: Standard mouse\n");
        dev_type = PS2_DEV_TYPE::MOUSE_STANDARD;
        break;

      case 0x03:
        KL_TRC_TRACE(TRC_LVL::FLOW, "ID: Wheel mouse\n");
        dev_type = PS2_DEV_TYPE::MOUSE_WITH_WHEEL;
        break;

      case 0x04:
        KL_TRC_TRACE(TRC_LVL::FLOW, "ID: 5-button mouse\n");
        dev_type = PS2_DEV_TYPE::MOUSE_5_BUTTON;
        break;

      case 0xAB:
        // Could be one of a couple of keyboard types.
        read_byte(response);
        KL_TRC_TRACE(TRC_LVL::EXTRA, "Some type of keyboard. Second response: ", response, "\n");
        switch(response)
        {
          case 0x41:
          case 0xC1:
          case 0x83:
          KL_TRC_TRACE(TRC_LVL::FLOW, "ID: MF2 Keyboard\n");
            dev_type = PS2_DEV_TYPE::KEYBOARD_MF2;
            break;

          default:
            KL_TRC_TRACE(TRC_LVL::FLOW, "ID: Unknown keyboard\n");
            dev_type = PS2_DEV_TYPE::UNKNOWN;
            break;
        }

        break;

      default:
        dev_type = PS2_DEV_TYPE::UNKNOWN;
        break;
    }
  }

  KL_TRC_EXIT;
  return dev_type;
}

/// @brief Construct a new PS/2 device object.
///
/// @param second_channel Is this device attached to the second channel of the controller?
///
/// @param dev_type The type of attached device.
///
/// @return Pointer to the device object.
gen_ps2_device *gen_ps2_controller_device::instantiate_device(bool second_channel, PS2_DEV_TYPE dev_type)
{
  KL_TRC_ENTRY;

  gen_ps2_device *res;

  switch(dev_type)
  {
    case PS2_DEV_TYPE::KEYBOARD_MF2:
      res = new ps2_keyboard_device(this, second_channel);
      break;

    case PS2_DEV_TYPE::MOUSE_5_BUTTON:
    case PS2_DEV_TYPE::MOUSE_STANDARD:
    case PS2_DEV_TYPE::MOUSE_WITH_WHEEL:
      res = new ps2_mouse_device(this, second_channel);
      break;

    default:
      res = new gen_ps2_device(this, second_channel);
  }

  KL_TRC_TRACE(TRC_LVL::FLOW, "Returning: ", res, "\n");
  KL_TRC_EXIT;

  return res;
}

/// @brief Read the configuration register from the controller.
///
/// @return Contents of the configuration register.
gen_ps2_controller_device::ps2_config_register gen_ps2_controller_device::read_config()
{
  KL_TRC_ENTRY;

  ps2_config_register config;
  send_ps2_command(PS2_CONST::READ_CONFIG, false, 0, true, config.raw);

  KL_TRC_EXIT;

  return config;
}

/// @brief Write the configuration register of this controller.
///
/// @param reg The configuration to write to the controller.
void gen_ps2_controller_device::write_config(gen_ps2_controller_device::ps2_config_register reg)
{
  KL_TRC_ENTRY;
  uint8_t response;

  send_ps2_command(PS2_CONST::WRITE_CONFIG, true, reg.raw, false, response);
  KL_TRC_EXIT;
}
