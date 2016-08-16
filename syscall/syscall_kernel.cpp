// The kernel's system call interface, on the kernel side.

#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"

// Called from the CPU-specific code whenever a system call is made.
void syscall_syscall()
{
  KL_TRC_ENTRY;
  panic("No system calls written yet!");
  KL_TRC_EXIT;
}
