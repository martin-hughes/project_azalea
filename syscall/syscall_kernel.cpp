// The kernel's system call interface, on the kernel side.

//#define ENABLE_TRACING

#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"

const void *syscall_pointers[] =
{ (void *)syscall_debug_output, };

const unsigned long syscall_max_idx = (sizeof(syscall_pointers) / sizeof(void *)) - 1;

/// Write desired output to the system debug output
///
/// Transcribe directly from a user mode process into the kernel debug output. There might not always be a debug output
/// compiled in (although there always is in these early builds), in which case this system call will do nothing.
///
/// @param msg The message to be output - it is output verbatim, so an ASCII string is best here. Must be a pointer to
///            user space memory, to prevent any jokers outputting kernel secrets!
///
/// @param length The number of bytes to output. Maximum 1024.
///
/// return ERR_CODE::INVALID_PARAM if either of the parameters isn't valid.
///        ERR_CODE::NO_ERROR otherwise (even if no output was actually made).
ERR_CODE syscall_debug_output(unsigned char *msg, unsigned long length)
{
  KL_TRC_ENTRY;

  if (length > 1024)
  {
    return ERR_CODE::INVALID_PARAM;
  }
  if (msg == nullptr)
  {
    return ERR_CODE::INVALID_PARAM;
  }
  if ((((unsigned long)msg) & 0x8000000000000000) != 0)
  {
    // Don't output from kernel space!
    return ERR_CODE::INVALID_PARAM;
  }

  for (int i = 0; i < length; i++)
  {
    kl_trc_char(msg[i]);
  }

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}
