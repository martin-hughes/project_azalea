[BITS 64]

; Project Azalea system call user interface.

; All system calls are basically the same - put the system call number in to RAX, swap RCX into R10 and then do the
; call.

%macro GENERIC_SYSCALL 2
GLOBAL %2
%2:
  mov rax, %1
  mov r10, rcx
  syscall
  ret
%endmacro

; The indices in this list MUST be the same as those in the az_pointers[] array in az_kernel.cpp!
GENERIC_SYSCALL 0, az_debug_output,

; Handle management.
GENERIC_SYSCALL 1, az_open_handle,
GENERIC_SYSCALL 2, az_create_obj_and_handle,
GENERIC_SYSCALL 3, az_close_handle,
GENERIC_SYSCALL 4, az_seek_handle,
GENERIC_SYSCALL 5, az_read_handle,
GENERIC_SYSCALL 6, az_get_handle_data_len,
GENERIC_SYSCALL 7, az_set_handle_data_len,
GENERIC_SYSCALL 8, az_write_handle,
GENERIC_SYSCALL 9, az_rename_object,
GENERIC_SYSCALL 10, az_delete_object,

GENERIC_SYSCALL 11, az_get_object_properties,
GENERIC_SYSCALL 12, az_enum_children,

; Message passing.
GENERIC_SYSCALL 13, az_register_for_mp,
GENERIC_SYSCALL 14, az_send_message,
GENERIC_SYSCALL 15, az_receive_message_details,
GENERIC_SYSCALL 16, az_receive_message_body,
GENERIC_SYSCALL 17, az_message_complete,

; Process & thread control
GENERIC_SYSCALL 18, az_create_process,
GENERIC_SYSCALL 19, az_set_startup_params,
GENERIC_SYSCALL 20, az_start_process,
GENERIC_SYSCALL 21, az_stop_process,
GENERIC_SYSCALL 22, az_destroy_process,
GENERIC_SYSCALL 23, az_exit_process,

GENERIC_SYSCALL 24, az_create_thread,
GENERIC_SYSCALL 25, az_start_thread,
GENERIC_SYSCALL 26, az_stop_thread,
GENERIC_SYSCALL 27, az_destroy_thread,
GENERIC_SYSCALL 28, az_exit_thread,

GENERIC_SYSCALL 29, az_thread_set_tls_base,

; Memory control,
GENERIC_SYSCALL 30, az_allocate_backing_memory,
GENERIC_SYSCALL 31, az_release_backing_memory,
GENERIC_SYSCALL 32, az_map_memory,
GENERIC_SYSCALL 33, az_unmap_memory,

; Thread synchronization
GENERIC_SYSCALL 34, az_wait_for_object,
GENERIC_SYSCALL 35, az_futex_op,
GENERIC_SYSCALL 36, az_create_mutex,
GENERIC_SYSCALL 37, az_release_mutex,
GENERIC_SYSCALL 38, az_create_semaphore,
GENERIC_SYSCALL 39, az_signal_semaphore,

; Timing
GENERIC_SYSCALL 40, az_get_system_clock,
GENERIC_SYSCALL 41, az_sleep_thread

; Other syscalls:
GENERIC_SYSCALL 42, az_yield
