/// @file
/// @brief Implement a driver for serial-port based terminals.

//#define ENABLE_TRACING

#include "serial_terminal.h"
#include "klib/klib.h"

#include "processor/x64/processor-x64-int.h"

const uint16_t TERM_COM_BASE_PORT = 0x2F8;

terms::serial::serial(std::shared_ptr<IWritable> keyboard_pipe) :
  terms::generic(keyboard_pipe)
{
  asm_proc_write_port(TERM_COM_BASE_PORT + 1, 0x00, 8); // Disable all interrupts
  asm_proc_write_port(TERM_COM_BASE_PORT + 3, 0x80, 8); // Enable DLAB (set baud rate divisor)
  asm_proc_write_port(TERM_COM_BASE_PORT + 0, 0x03, 8); // Set divisor to 3 (lo byte) 38400 baud
  asm_proc_write_port(TERM_COM_BASE_PORT + 1, 0x00, 8); //                  (hi byte)
  asm_proc_write_port(TERM_COM_BASE_PORT + 3, 0x03, 8); // 8 bits, no parity, one stop bit
  asm_proc_write_port(TERM_COM_BASE_PORT + 2, 0xC7, 8); // Enable FIFO, clear them, with 14-byte threshold
  asm_proc_write_port(TERM_COM_BASE_PORT + 4, 0x0B, 8); // IRQs enabled, RTS/DSR set
}

void terms::serial::write_raw_string(const char *out_string, uint16_t num_chars)
{
  KL_TRC_ENTRY;

  for (uint16_t i = 0; i < num_chars; i++)
  {
    while (!(bool(asm_proc_read_port(TERM_COM_BASE_PORT + 5, 8) & 0x20)))
    {
      //spin!
    }
    asm_proc_write_port(TERM_COM_BASE_PORT, (uint64_t)(out_string[i]), 8);
  }

  KL_TRC_EXIT;
}

void terms::serial::temp_read_port()
{
  char c;

  KL_TRC_ENTRY;

  while ((asm_proc_read_port(TERM_COM_BASE_PORT + 5, 8) & 1) == 1)
  {
    c = static_cast<uint8_t>(asm_proc_read_port(TERM_COM_BASE_PORT, 8));
    KL_TRC_TRACE(TRC_LVL::FLOW, "char: ", c, "\n");
    this->handle_character(c);
  }

  KL_TRC_EXIT;
}

