/// @file
/// @brief The kernel's system call table and a couple of generic system calls.

//#define ENABLE_TRACING

#include <memory>
#include <cstring>

#include "kernel_all.h"
#include "syscall_kernel-int.h"

// The indicies of the pointers in this table MUST match the indicies given in syscall_user_low-x64.asm!
/// @brief Main system call table.
///
const void *syscall_pointers[] =
    { (void *)az_debug_output,

      // Handle management.
      (void *)az_open_handle,
      (void *)az_create_obj_and_handle,
      (void *)az_close_handle,
      (void *)az_seek_handle,
      (void *)az_read_handle,
      (void *)az_get_handle_data_len,
      (void *)az_set_handle_data_len,
      (void *)az_write_handle,
      (void *)az_rename_object,
      (void *)az_delete_object,

      (void *)az_get_object_properties,
      (void *)az_enum_children,

      // Message passing.
      (void *)az_register_for_mp,
      (void *)az_send_message,
      (void *)az_receive_message_details,
      (void *)az_receive_message_body,
      (void *)az_message_complete,

      // Process & thread control
      (void *)az_create_process,
      (void *)az_set_startup_params,
      (void *)az_start_process,
      (void *)az_stop_process,
      (void *)az_destroy_process,
      (void *)az_exit_process,

      (void *)az_create_thread,
      (void *)az_start_thread,
      (void *)az_stop_thread,
      (void *)az_destroy_thread,
      (void *)az_exit_thread,

      (void *)az_thread_set_tls_base,

      // Memory control,
      (void *)az_allocate_backing_memory,
      (void *)az_release_backing_memory,
      (void *)az_map_memory,
      (void *)az_unmap_memory,

      // Thread synchronization
      (void *)az_wait_for_object,
      (void *)az_futex_op,
      (void *)az_create_mutex,
      (void *)az_release_mutex,
      (void *)az_create_semaphore,
      (void *)az_signal_semaphore,

      // Timing syscalls:
      (void *)az_get_system_clock,
      (void *)az_sleep_thread,

      // Other syscalls:
      (void *)az_yield,
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
ERR_CODE az_debug_output(const char *msg, uint64_t length)
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
ERR_CODE az_thread_set_tls_base(TLS_REGISTERS reg, uint64_t value)
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
    case TLS_REGISTERS::GS:
      proc_set_tls_register(reg, value);
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
