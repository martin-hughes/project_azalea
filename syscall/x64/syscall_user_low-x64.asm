[BITS 64]

; Project Azalea system call user interface.

; All system calls are basically the same - put the system call number in to RAX, swap RCX into R10 and then do the;
; call.
%macro GENERIC_SYSCALL 1
  mov rax, %1
  mov r10, rcx
  syscall
  ret
%endmacro

GLOBAL syscall_debug_output
syscall_debug_output:
  GENERIC_SYSCALL 0
