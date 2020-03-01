[BITS 64]
SEGMENT .pretext

; This file is called from entry-x86.asm, which executes the 32-bit code needed to get as far as 64-bit long mode, and
; then passes control to pre_main_64. This resets the stack and makes some final adjustments to the page tables before
; passing control into the main part of the kernel.

EXTERN main
GLOBAL pre_main_64
pre_main_64:

; Adjust stack pointer to point at upper memory, and align it to 16-byte boundary.
mov rax, rsp
mov rbx, 0xFFFFFFFF00000000
or rax, rbx
and rax, qword 0xFFFFFFFFFFFFFFF0
mov rsp, rax

; Compute virtual address that can be edited to affect mapping of virtual address 0xFFFFFFFFFFFE0000.
EXTERN working_table_va_entry_addr
EXTERN mm_data_page_directory
mov rax, mm_data_page_directory
add rax, QWORD 0xFF8
mov rbx, QWORD 0xFFFFFFFF00000000
or rax, rbx
mov rbx, working_table_va_entry_addr
mov [rbx], rax

mov rbp, 0

; Call the kernel proper.
mov rax, main
call rax
