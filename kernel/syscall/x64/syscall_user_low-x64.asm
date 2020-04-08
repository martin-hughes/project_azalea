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

; The indices in this list MUST be the same as those in the syscall_pointers[] array in syscall_kernel.cpp!
GENERIC_SYSCALL 0, syscall_debug_output,

; Handle management.
GENERIC_SYSCALL 1, syscall_open_handle,
GENERIC_SYSCALL 2, syscall_create_obj_and_handle,
GENERIC_SYSCALL 3, syscall_close_handle,
GENERIC_SYSCALL 4, syscall_seek_handle,
GENERIC_SYSCALL 5, syscall_read_handle,
GENERIC_SYSCALL 6, syscall_get_handle_data_len,
GENERIC_SYSCALL 7, syscall_set_handle_data_len,
GENERIC_SYSCALL 8, syscall_write_handle,
GENERIC_SYSCALL 9, syscall_rename_object,
GENERIC_SYSCALL 10, syscall_delete_object,

GENERIC_SYSCALL 11, syscall_get_object_properties,
GENERIC_SYSCALL 12, syscall_enum_children,

; Message passing.
GENERIC_SYSCALL 13, syscall_register_for_mp,
GENERIC_SYSCALL 14, syscall_send_message,
GENERIC_SYSCALL 15, syscall_receive_message_details,
GENERIC_SYSCALL 16, syscall_receive_message_body,
GENERIC_SYSCALL 17, syscall_message_complete,

; Process & thread control
GENERIC_SYSCALL 18, syscall_create_process,
GENERIC_SYSCALL 19, syscall_set_startup_params,
GENERIC_SYSCALL 20, syscall_start_process,
GENERIC_SYSCALL 21, syscall_stop_process,
GENERIC_SYSCALL 22, syscall_destroy_process,
GENERIC_SYSCALL 23, syscall_exit_process,

GENERIC_SYSCALL 24, syscall_create_thread,
GENERIC_SYSCALL 25, syscall_start_thread,
GENERIC_SYSCALL 26, syscall_stop_thread,
GENERIC_SYSCALL 27, syscall_destroy_thread,
GENERIC_SYSCALL 28, syscall_exit_thread,

GENERIC_SYSCALL 29, syscall_thread_set_tls_base,

; Memory control,
GENERIC_SYSCALL 30, syscall_allocate_backing_memory,
GENERIC_SYSCALL 31, syscall_release_backing_memory,
GENERIC_SYSCALL 32, syscall_map_memory,
GENERIC_SYSCALL 33, syscall_unmap_memory,

; Thread synchronization
GENERIC_SYSCALL 34, syscall_wait_for_object,
GENERIC_SYSCALL 35, syscall_futex_op,
GENERIC_SYSCALL 36, syscall_create_mutex,
GENERIC_SYSCALL 37, syscall_release_mutex,
GENERIC_SYSCALL 38, syscall_create_semaphore,
GENERIC_SYSCALL 39, syscall_signal_semaphore,

; Timing
GENERIC_SYSCALL 40, syscall_get_system_clock,
GENERIC_SYSCALL 41, syscall_sleep_thread

; Other syscalls:
GENERIC_SYSCALL 42, syscall_yield
