// KLib Kernel tracing library. In the live version of the kernel, these
// currently do nothing.

#include "tracing.h"
#include "tracing_internal.h"

// Only one of these can be defined at a time, or duplicate definitions will occur.
// Note that tracing by serial port assumes the normal port values for COM1, and seizes it completely! (Which may be
// undesirable)
//#define KL_TRACE_BY_MAGIC_PORT
#define KL_TRACE_BY_SERIAL_PORT


#ifdef KL_TRACE_BY_SERIAL_PORT

#include "processor/x64/processor-x64-int.h"
const unsigned short TRC_COM1_BASE_PORT = 0x3F8;

bool kl_trc_serial_port_ready()
{
  return bool(asm_proc_read_port(TRC_COM1_BASE_PORT + 5, 8) & 0x20);
}

#endif

void kl_trc_init_tracing()
{
#ifdef KL_TRACE_BY_MAGIC_PORT
  // Currently needs no initialisation
#endif

#ifdef KL_TRACE_BY_SERIAL_PORT
  asm_proc_write_port(TRC_COM1_BASE_PORT + 1, 0x00, 8); // Disable all interrupts
  asm_proc_write_port(TRC_COM1_BASE_PORT + 3, 0x80, 8); // Enable DLAB (set baud rate divisor)
  asm_proc_write_port(TRC_COM1_BASE_PORT + 0, 0x03, 8); // Set divisor to 3 (lo byte) 38400 baud
  asm_proc_write_port(TRC_COM1_BASE_PORT + 1, 0x00, 8); //                  (hi byte)
  asm_proc_write_port(TRC_COM1_BASE_PORT + 3, 0x03, 8); // 8 bits, no parity, one stop bit
  asm_proc_write_port(TRC_COM1_BASE_PORT + 2, 0xC7, 8); // Enable FIFO, clear them, with 14-byte threshold
  asm_proc_write_port(TRC_COM1_BASE_PORT + 4, 0x0B, 8); // IRQs enabled, RTS/DSR set
#endif
}

void kl_trc_output_argument(char const *str)
{
#ifdef KL_TRACE_BY_SERIAL_PORT
  while(*str != 0)
  {
    while(!kl_trc_serial_port_ready())
    {
      //spin!
    }

    asm_proc_write_port(TRC_COM1_BASE_PORT, *str, 8);
    str++;
  }
#endif
#ifdef KL_TRACE_BY_MAGIC_PORT
  asm_trc_dbg_port_output_string(str);
#endif
}

void kl_trc_output_argument(unsigned long value)
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

  kl_trc_output_argument(buf);
}

void kl_trc_output_argument()
{
  // This function is called when there are no more arguments for kl_trc_output_argument, so it doesn't need to do
  // anything
}
