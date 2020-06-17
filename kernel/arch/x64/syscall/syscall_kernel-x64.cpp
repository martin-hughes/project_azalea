/// @file
/// @brief x64 specific functions for the kernel to manage its system call interface.

//#define ENABLE_TRACING

#include "syscall_kernel-x64.h"
#include "tracing.h"

/// @brief Prepare the system call interface for use.
///
void syscall_gen_init()
{
  KL_TRC_ENTRY;

  asm_syscall_x64_prepare();

  KL_TRC_EXIT;
}

