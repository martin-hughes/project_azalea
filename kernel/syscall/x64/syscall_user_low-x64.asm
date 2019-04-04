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

; The indicies in this list MUST be the same as those in the syscall_pointers[] array in syscall_kernel.cpp!
GENERIC_SYSCALL 0, syscall_debug_output
GENERIC_SYSCALL 1, syscall_open_handle
GENERIC_SYSCALL 2, syscall_close_handle
GENERIC_SYSCALL 3, syscall_read_handle
GENERIC_SYSCALL 4, syscall_get_handle_data_len
GENERIC_SYSCALL 5, syscall_write_handle
GENERIC_SYSCALL 6, syscall_register_for_mp
GENERIC_SYSCALL 7, syscall_send_message
GENERIC_SYSCALL 8, syscall_receive_message_details
GENERIC_SYSCALL 9, syscall_receive_message_body
GENERIC_SYSCALL 10, syscall_message_complete

; Process & thread control
GENERIC_SYSCALL 11, syscall_create_process
GENERIC_SYSCALL 12, syscall_start_process
GENERIC_SYSCALL 13, syscall_stop_process
GENERIC_SYSCALL 14, syscall_destroy_process
GENERIC_SYSCALL 15, syscall_exit_process

GENERIC_SYSCALL 16, syscall_create_thread
GENERIC_SYSCALL 17, syscall_start_thread
GENERIC_SYSCALL 18, syscall_stop_thread
GENERIC_SYSCALL 19, syscall_destroy_thread
GENERIC_SYSCALL 20, syscall_exit_thread

GENERIC_SYSCALL 21, syscall_thread_set_tls_base

; Memory control
GENERIC_SYSCALL 22, syscall_allocate_backing_memory
GENERIC_SYSCALL 23, syscall_release_backing_memory
GENERIC_SYSCALL 24, syscall_map_memory
GENERIC_SYSCALL 25, syscall_unmap_memory

; Thread synchronization
GENERIC_SYSCALL 26, syscall_wait_for_object
GENERIC_SYSCALL 27, syscall_futex_wait
GENERIC_SYSCALL 28, syscall_futex_wake

; New syscalls
GENERIC_SYSCALL 29, syscall_create_obj_and_handle
GENERIC_SYSCALL 30, syscall_set_handle_data_len
GENERIC_SYSCALL 31, syscall_set_startup_params
GENERIC_SYSCALL 32, syscall_get_system_clock
GENERIC_SYSCALL 33, syscall_rename_object
GENERIC_SYSCALL 34, syscall_delete_object
