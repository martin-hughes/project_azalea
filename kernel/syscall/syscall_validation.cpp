/// @file
/// @brief Provides validation of parameters sent by the system call interface

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "syscall_kernel-int.h"

/// @brief Is addr a user-mode address or not?
///
/// User mode addresses are those in the lower half of the address space. Kernel mode addresses are in the upper half.
/// Kernel mode addresses are restricted to kernel mode accesses only.
///
/// @param addr The address to check.
///
/// @return True if addr is user-mode, false otherwise.
bool syscall_v_is_um_address(const void *addr)
{
  uint64_t addr_l = reinterpret_cast<uint64_t>(addr);
  return !(addr_l & 0x8000000000000000ULL);
}

/// @brief Is the buffer given by base -> base + length entirely within user space or not?
///
/// @param base The base address of the buffer under check.
///
/// @param length The length of the buffer under check
///
/// @return True if the buffer is in user space, false otherwise.
bool syscall_v_is_um_buffer(const void *base, uint64_t length)
{
  uint64_t base_l = reinterpret_cast<uint64_t>(base);

  return (!(length & 0x8000000000000000ULL)) &&
         (!(base_l & 0x8000000000000000ULL)) &&
         (!((base_l + length) & 0x8000000000000000ULL));
}
