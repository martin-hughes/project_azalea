// KLib Kernel tracing library. In the live version of the kernel, these will do nothing.

#include "tracing.h"

// Only one of these can be defined at a time, or duplicate definitions will occur.
// Note that tracing by serial port assumes the normal port values for COM1, and seizes it completely! (Which may be
// undesirable)
//#define KL_TRACE_BY_MAGIC_PORT
#define KL_TRACE_BY_SERIAL_PORT

#include "processor/x64/processor-x64-int.h"
const unsigned short TRC_COM1_BASE_PORT = 0x3F8;
const unsigned short TRC_MAGIC_PORT = 0xE9;

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
  asm_proc_write_port(TRC_COM1_BASE_PORT, (unsigned long)c, 8);
#endif
#ifdef KL_TRACE_BY_MAGIC_PORT
  asm_proc_write_port(TRC_MAGIC_PORT, (unsigned long)c, 8);
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

void kl_trc_output_int_argument(unsigned long value)
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

void kl_trc_output_kl_string_argument(kl_string &str)
{
  for (unsigned long x = 0; x < str.length(); x++)
  {
    kl_trc_char(str[x]);
  }
}

void kl_trc_output_err_code_argument(ERR_CODE ec)
{
  const char *msg = nullptr;
  switch(ec)
  {
    case ERR_CODE::NO_ERROR:
      msg = "No error";
      break;

    case ERR_CODE::UNKNOWN:
      msg = "Unknown error";
      break;

    case ERR_CODE::SYSCALL_INVALID_IDX:
      msg = "Invalid system call number";
      break;

    case ERR_CODE::NOT_FOUND:
      msg = "Not found";
      break;

    case ERR_CODE::WRONG_TYPE:
      msg = "Wrong type";
      break;

    case ERR_CODE::ALREADY_EXISTS:
      msg = "Already exists";
      break;

    case ERR_CODE::INVALID_NAME:
      msg = "Invalid name";
      break;

    case ERR_CODE::INVALID_PARAM:
      msg = "Invalid Parameter";
      break;

    case ERR_CODE::INVALID_OP:
      msg = "Invalid operation";
      break;

    case ERR_CODE::DEVICE_FAILED:
      msg = "Device failed";
      break;

    case ERR_CODE::STORAGE_ERROR:
      msg = "Storage error";
      break;

    default:
      break;
  }

  if (msg != nullptr)
  {
    kl_trc_output_str_argument(msg);
  }
  else
  {
    kl_trc_output_str_argument("Unknown code: ");
    kl_trc_output_int_argument((unsigned long)(ec));
  }
}
