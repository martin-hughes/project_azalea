// x64 specific functions for the kernel to manage its system call interface.

//#define ENABLE_TRACING

#include "syscall/x64/syscall_kernel-x64.h"
#include "syscall/x64/syscall_kernel-x64-int.h"
#include "klib/klib.h"

// Prepare the system call interface for use.
void syscall_gen_init()
{
  KL_TRC_ENTRY;

  asm_syscall_x64_prepare();

  KL_TRC_EXIT;
}

// The C-level part of the system call interface:
void syscall_x64_kernel_syscall()
{
  KL_TRC_ENTRY;

  //panic("No system call interface defined!");

  KL_TRC_EXIT;
}
