#ifndef SYSCALL_KERNEL_X64_INT_H
#define SYSCALL_KERNEL_X64_INT_H

extern "C" void asm_syscall_x64_prepare();
extern "C" void syscall_x64_kernel_syscall();

extern void *syscall_x64_kernel_stack;

#endif
