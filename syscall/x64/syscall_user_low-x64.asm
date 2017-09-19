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
