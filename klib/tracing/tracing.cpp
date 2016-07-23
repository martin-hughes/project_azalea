// KLib Kernel tracing library. In the live version of the kernel, these
// currently do nothing.

#include "tracing.h"
#include "tracing_internal.h"

void kl_trc_init_tracing()
{
  // Currently needs no initialization
}

void kl_trc_trace(unsigned long level, const char *message)
{
  // TODO: Implement tracing better. At the moment, only qemu logging is supported.
  asm_trc_dbg_port_output_string(message);
}

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
