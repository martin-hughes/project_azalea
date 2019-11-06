/// @file
/// @brief System call libaray internal functions.

#pragma once

extern const void *syscall_pointers[];
extern const uint64_t syscall_max_idx;

/// @brief Convenience wrapper around syscall_is_um_address
///
/// @param x Value to convert to pointer type and give to syscall_is_um_address
///
/// @return True if x is a user-mode address, false if it's a kernel-mode address
#define SYSCALL_IS_UM_ADDRESS(x) syscall_is_um_address(reinterpret_cast<const void *>((x)))
bool syscall_is_um_address(const void *addr);
