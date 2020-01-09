/// @file
/// @brief KLib Kernel tracing library.
///
/// In the release version of the kernel, these will probably do nothing.

#include <string>

#include "tracing.h"

// The destination of tracing is controlled by defining one of the following possible flags at compile time.
// The flag itself is currently passed directly to the compiler.
// - KL_TRACE_BY_MAGIC_PORT - traces to the qemu magic CPU port 0xE9
// - KL_TRACE_BY_SERIAL_PORT - traces to COM1
// - KL_TRACE_BY_STDOUT - traces to stdout. Given that stdout doesn't really exist in the kernel environment it should
//   be fairly easy to understand that this only works in the test code!

// None of these functions are documented because they're reasonable self-explanatory.
/// @cond

#include "processor/x64/processor-x64-int.h"
#include "devices/device_interface.h"

#ifdef KL_TRACE_BY_SERIAL_PORT
const uint16_t TRC_COM1_BASE_PORT = 0x3F8;
#endif
#ifdef KL_TRACE_BY_MAGIC_PORT
const uint16_t TRC_MAGIC_PORT = 0xE9;
#endif

#ifdef KL_TRACE_BY_STDOUT
#include <iostream>
using namespace std;
#endif

#ifdef KL_TRACE_BY_SERIAL_PORT

bool kl_trc_serial_port_ready()
{
  return bool(asm_proc_read_port(TRC_COM1_BASE_PORT + 5, 8) & 0x20);
}

#endif

void kl_trc_char(unsigned char c)
{
#ifdef KL_TRACE_BY_SERIAL_PORT
  while (!kl_trc_serial_port_ready())
  {
    //spin!
  }
  asm_proc_write_port(TRC_COM1_BASE_PORT, (uint64_t)c, 8);
#endif
#ifdef KL_TRACE_BY_MAGIC_PORT
  asm_proc_write_port(TRC_MAGIC_PORT, (uint64_t)c, 8);
#endif
#ifdef KL_TRACE_BY_STDOUT
  cout << c;
#endif
}

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

void kl_trc_output_int_argument(uint64_t value)
{
  char buf[19] = "0x0000000000000000";
  char temp = 0;

  for (int i = 0; i < 16; i++)
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

  kl_trc_output_str_argument((char const *)buf);
}

void kl_trc_output_str_argument(char const *str)
{
  while (*str != 0)
  {
    kl_trc_char(*str);
    str++;
  }
}

void kl_trc_output_std_string_argument(std::string &str)
{
  for (uint64_t x = 0; x < str.length(); x++)
  {
    kl_trc_char(str[x]);
  }
}

void kl_trc_output_err_code_argument(ERR_CODE ec)
{
  const char *msg = nullptr;
  msg = azalea_lookup_err_code(ec);

  if (msg != nullptr)
  {
    kl_trc_output_str_argument(msg);
  }
  else
  {
    kl_trc_output_str_argument("Unknown code: ");
    kl_trc_output_int_argument((uint64_t)(ec));
  }
}

#define DS_LOOKUP(s, n) \
  case s : \
    kl_trc_output_str_argument( n ); \
    break

void kl_trc_output_dev_status_argument(DEV_STATUS ds)
{
  switch (ds)
  {
  DS_LOOKUP(DEV_STATUS::UNKNOWN, "Unknown");
  DS_LOOKUP(DEV_STATUS::STOPPED, "Stopped");
  DS_LOOKUP(DEV_STATUS::STARTING, "Starting");
  DS_LOOKUP(DEV_STATUS::STOPPING, "Stopping");
  DS_LOOKUP(DEV_STATUS::OK, "Running OK");
  DS_LOOKUP(DEV_STATUS::RESET, "Resetting");
  DS_LOOKUP(DEV_STATUS::FAILED, "Failed");
  }
}

/// @endcond
