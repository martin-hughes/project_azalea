#ifndef SYSCALL_INT_KERNEL_H
#define SYSCALL_INT_KERNEL_H

extern const void *syscall_pointers[];
extern const unsigned long syscall_max_idx;

#define SYSCALL_IS_UM_ADDRESS(x) syscall_is_um_address(reinterpret_cast<const void *>((x)))
bool syscall_is_um_address(const void *addr);

#endif
