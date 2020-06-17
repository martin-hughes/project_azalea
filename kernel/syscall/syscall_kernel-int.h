/// @file
/// @brief System call libaray internal functions.

#pragma once

#include <stdint.h>

#include "azalea/syscall.h"

extern const void *syscall_pointers[];
extern const uint64_t syscall_max_idx;

/// @brief Convenience wrapper around syscall_v_is_um_address
///
/// @param x Value to convert to pointer type and give to syscall_v_is_um_address
///
/// @return True if x is a user-mode address, false if it's a kernel-mode address
#define SYSCALL_IS_UM_ADDRESS(x) syscall_v_is_um_address(reinterpret_cast<const void *>((x)))
bool syscall_v_is_um_address(const void *addr);

/// @brief Convenience wrapper around syscall_v_is_um_buffer
///
/// @param x Base address of buffer under test
///
/// @param y Length of buffer under test
///
/// @return True if x->(x+y) falls entirely within user space, false otherwise
#define SYSCALL_IS_UM_BUFFER(x, y) syscall_v_is_um_buffer(reinterpret_cast<const void *>((x)), (y))
bool syscall_v_is_um_buffer(const void *base, uint64_t length);
