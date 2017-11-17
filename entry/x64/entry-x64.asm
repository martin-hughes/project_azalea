[BITS 64]
SEGMENT .pretext

; This file is called from entry-x86.asm, which executes the 32-bit code needed to get as far as 64-bit long mode, and
; then passes control to pre_main_64. This resets the stack and makes some final adjustments to the page tables before
; passing control into the main part of the kernel.

EXTERN main
GLOBAL pre_main_64
pre_main_64:

; Adjust stack pointer to point at upper memory.
mov rax, rsp
mov rbx, 0xFFFFFFFF00000000
or rax, rbx
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

; Jump in to the kernel proper. This indirect jump is necessary because of the way the .pretext segment is linked to be
; in lower memory, but the other segments are linked for upper memory. Direct jumps don't work for such a long jump.
mov rax, main
jmp rax
