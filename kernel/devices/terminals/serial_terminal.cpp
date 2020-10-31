/// @file
/// @brief Implement a driver for serial-port based terminals.

//#define ENABLE_TRACING

#include "kernel_all.h"
#include "serial_terminal.h"

#include "../arch/x64/processor/processor-x64-int.h"

/// @brief Create a new terminal that operates over a serial port.
///
/// @param keyboard_pipe The pipe to write keypresses in to (which becomes stdin for the attached process).
///
/// @param output_port_s Shared pointer to an object to write data to send it towards the terminal.
///
/// @param input_port_s Shared pointer to an object to read data from when it is sent by the terminal.
terms::serial::serial(std::shared_ptr<IWriteImmediate> keyboard_pipe,
                      std::shared_ptr<IWriteImmediate> output_port_s,
                      std::shared_ptr<IReadImmediate> input_port_s) :
  terms::generic{keyboard_pipe},
  output_port{output_port_s},
  input_port{input_port_s}
{

}

void terms::serial::handle_pipe_new_data(std::unique_ptr<msg::root_msg> &message)
{
  KL_TRC_ENTRY;

  // Either stdout_reader has data or input_port has data.
  if (message->message_id == SM_PIPE_NEW_DATA)
  {
    const uint64_t buffer_size = 10;
    char buffer[buffer_size];
    uint64_t bytes_read;

    KL_TRC_TRACE(TRC_LVL::FLOW, "Passing data from pipe to screen\n");

    if (stdout_reader)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Deal with stdout data\n");
      while(1)
      {
        if (stdout_reader->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
            ERR_CODE::NO_ERROR)
        {
          if (bytes_read != 0)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
            this->write_string(buffer, bytes_read);
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading as end of stream\n");
            break;
          }
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading due error\n");
          break;
        }
      }
    }

    if (input_port)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Passing data from serial port towards user space\n");
      while(1)
      {
        if (input_port->read_bytes(0, buffer_size, reinterpret_cast<uint8_t *>(buffer), buffer_size, bytes_read) ==
            ERR_CODE::NO_ERROR)
        {
          if (bytes_read != 0)
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Write output\n");
            for (uint64_t i = 0; i < bytes_read; i++)
            {
              KL_TRC_TRACE(TRC_LVL::FLOW, "Write character: ", buffer[i], "\n");
              this->handle_character(buffer[i]);
            }
          }
          else
          {
            KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading as end of stream\n");
            break;
          }
        }
        else
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Stop reading due error\n");
          break;
        }
      }
    }
  }

  KL_TRC_EXIT;
}

void terms::serial::write_raw_string(const char *out_string, uint16_t num_chars)
{
  uint64_t bw{0};

  KL_TRC_ENTRY;

  if (get_device_status() != OPER_STATUS::OK)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Ignore request while not running\n");
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Handle request while running\n");
    if (output_port)
    {
      output_port->write_bytes(0, num_chars, reinterpret_cast<const uint8_t *>(out_string), num_chars, bw);
    }
  }

  KL_TRC_EXIT;
}
