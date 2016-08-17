// KLib Kernel tracing library. In the live version of the kernel, these
// currently do nothing.

#include "tracing.h"
#include "tracing_internal.h"

// Only one of these can be defined at a time, or duplicate definitions will occur.
// Note that tracing by serial port assumes the normal port values for COM1, and seizes it completely! (Which may be
// undesirable)
//#define KL_TRACE_BY_MAGIC_PORT
#define KL_TRACE_BY_SERIAL_PORT

#ifdef KL_TRACE_BY_MAGIC_PORT
void kl_trc_init_tracing()
{
  // Currently needs no initialisation
}

void kl_trc_trace(unsigned long level, const char *message)
{
  asm_trc_dbg_port_output_string(message);
}

#endif

#ifdef KL_TRACE_BY_SERIAL_PORT

#include "processor/x64/processor-x64-int.h"

const unsigned short TRC_COM1_BASE_PORT = 0x3F8;

void kl_trc_init_tracing()
{
  // Initialise the serial port. This is modified from http://wiki.osdev.org/Serial_Ports
  asm_proc_write_port(TRC_COM1_BASE_PORT + 1, 0x00, 8); // Disable all interrupts
  asm_proc_write_port(TRC_COM1_BASE_PORT + 3, 0x80, 8); // Enable DLAB (set baud rate divisor)
  asm_proc_write_port(TRC_COM1_BASE_PORT + 0, 0x03, 8); // Set divisor to 3 (lo byte) 38400 baud
  asm_proc_write_port(TRC_COM1_BASE_PORT + 1, 0x00, 8); //                  (hi byte)
  asm_proc_write_port(TRC_COM1_BASE_PORT + 3, 0x03, 8); // 8 bits, no parity, one stop bit
  asm_proc_write_port(TRC_COM1_BASE_PORT + 2, 0xC7, 8); // Enable FIFO, clear them, with 14-byte threshold
  asm_proc_write_port(TRC_COM1_BASE_PORT + 4, 0x0B, 8); // IRQs enabled, RTS/DSR set
}

bool kl_trc_serial_port_ready()
{
  return bool(asm_proc_read_port(TRC_COM1_BASE_PORT + 5, 8) & 0x20);
}

void kl_trc_trace(unsigned long level, const char *message)
{
  while(*message != 0)
  {
    while(!kl_trc_serial_port_ready())
    {
      //spin!
    }

    asm_proc_write_port(TRC_COM1_BASE_PORT, *message, 8);
    message++;
  }
}

#endif

void kl_trc_trace(unsigned long level, unsigned long value)
{
  char buf[19] = "0x0000000000000000";
  char temp = 0;

  for (int i=0; i < 16; i++)
  {
    temp = (char)(value & 0x0F);
    if (temp < 10)
    {
      temp = '0' + temp;
    }
    else
    {
      temp = 'A' + (temp - 10);
    }
    buf[17 - i] = temp;
    value >>= 4;
  }

  kl_trc_trace(level, buf);
}
