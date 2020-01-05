/// @file
/// @brief The kernel's system call table and a couple of generic system calls.

//#define ENABLE_TRACING

#include "user_interfaces/syscall.h"
#include "syscall/syscall_kernel.h"
#include "syscall/syscall_kernel-int.h"
#include "klib/klib.h"
#include "system_tree/system_tree.h"
#include "system_tree/fs/fs_file_interface.h"
#include "object_mgr/object_mgr.h"
#include "processor/x64/processor-x64.h"

#include <memory>
#include <cstring>

// The indicies of the pointers in this table MUST match the indicies given in syscall_user_low-x64.asm!
/// @brief Main system call table.
///
const void *syscall_pointers[] =
    { (void *)syscall_debug_output,

      // Handle management.
      (void *)syscall_open_handle,
      (void *)syscall_close_handle,
      (void *)syscall_read_handle,
      (void *)syscall_get_handle_data_len,
      (void *)syscall_write_handle,

      // Message passing.
      (void *)syscall_register_for_mp,
      (void *)syscall_send_message,
      (void *)syscall_receive_message_details,
      (void *)syscall_receive_message_body,
      (void *)syscall_message_complete,

      // Process & thread control
      (void *)syscall_create_process,
      (void *)syscall_start_process,
      (void *)syscall_stop_process,
      (void *)syscall_destroy_process,
      (void *)syscall_exit_process,

      (void *)syscall_create_thread,
      (void *)syscall_start_thread,
      (void *)syscall_stop_thread,
      (void *)syscall_destroy_thread,
      (void *)syscall_exit_thread,

      (void *)syscall_thread_set_tls_base,

      // Memory control,
      (void *)syscall_allocate_backing_memory,
      (void *)syscall_release_backing_memory,
      (void *)syscall_map_memory,
      (void *)syscall_unmap_memory,

      // Thread synchronization
      (void *)syscall_wait_for_object,
      (void *)syscall_futex_wait,
      (void *)syscall_futex_wake,

      // New syscalls:
      (void *)syscall_create_obj_and_handle,
      (void *)syscall_set_handle_data_len,
      (void *)syscall_set_startup_params,
      (void *)syscall_get_system_clock,
      (void *)syscall_rename_object,
      (void *)syscall_delete_object,
      (void *)syscall_get_object_properties,
      (void *)syscall_seek_handle,
      (void *)syscall_create_mutex,
      (void *)syscall_release_mutex,
      (void *)syscall_create_semaphore,
      (void *)syscall_signal_semaphore,
    };

/// @brief The number of known system calls.
///
const uint64_t syscall_max_idx = (sizeof(syscall_pointers) / sizeof(void *)) - 1;

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
/// @return ERR_CODE::INVALID_PARAM if either of the parameters isn't valid.
///         ERR_CODE::NO_ERROR otherwise (even if no output was actually made).
ERR_CODE syscall_debug_output(const char *msg, uint64_t length)
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
  if (!SYSCALL_IS_UM_ADDRESS(msg))
  {
    // Don't output from kernel space!
    return ERR_CODE::INVALID_PARAM;
  }

  for (uint32_t i = 0; i < length; i++)
  {
    kl_trc_char(msg[i]);
  }

  KL_TRC_EXIT;

  return ERR_CODE::NO_ERROR;
}

/// @brief Configure the base address of TLS for this thread.
///
/// Threads generally define their thread-local storage relative to either FS or GS. It is difficult for them to set
/// the base address of those registers in user-mode, so this system call allows the kernel to do it on their behalf.
///
/// It should be noted that modern x64 processors allow user mode threads to do this via WRGSBASE (etc.) but QEMU
/// doesn't support these instructions, so we can't enable it in Azalea yet.
///
/// @param reg Which register to set.
///
/// @param value The value to load into the base of the required register.
///
/// @return ERR_CODE::INVALID_PARAM if either reg isn't valid, or value either isn't canonical or in user-space.
ERR_CODE syscall_thread_set_tls_base(TLS_REGISTERS reg, uint64_t value)
{
  KL_TRC_ENTRY;

  ERR_CODE result = ERR_CODE::NO_ERROR;

  if (!SYSCALL_IS_UM_ADDRESS(value) || !mem_is_valid_virt_addr(value))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid base address\n");
    result = ERR_CODE::INVALID_PARAM;
  }
  else
  {
    switch(reg)
    {
    case TLS_REGISTERS::FS:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Setting FS base to ", value, "\n");
      proc_write_msr (PROC_X64_MSRS::IA32_FS_BASE, value);
      break;

    case TLS_REGISTERS::GS:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Writing GS base to ", value, "\n");
      proc_write_msr (PROC_X64_MSRS::IA32_GS_BASE, value);
      break;

    default:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unknown register\n");
      result = ERR_CODE::INVALID_PARAM;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}
