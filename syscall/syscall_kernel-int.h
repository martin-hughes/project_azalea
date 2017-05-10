#ifndef SYSCALL_INT_KERNEL_H
#define SYSCALL_INT_KERNEL_H

#include "klib/misc/error_codes.h"

extern "C" ERR_CODE syscall_debug_output(unsigned char *msg, unsigned long length);

extern const void *syscall_pointers[];
extern const unsigned long syscall_max_idx;

#endif
