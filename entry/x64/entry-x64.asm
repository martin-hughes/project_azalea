[BITS 64]
SEGMENT .pretext

; Set up a simple page map indicating that the kernel lives in upper memory,
; then jump in to the kernel.

EXTERN main
GLOBAL pre_main
pre_main:

; Fill in appropriate page table entries. The kernel is loaded at 1MB upwards,
; and is linked against 0xFFFFFFFF00100000. For the time being, we still need
; to be able to use the lower-half addresses too, otherwise the system fails
; immediately after starting paging because the next instruction can't be found.
;
; We use 2MB pages, so the relevant entries to fill are:
; - 0x000 of the PML4. (Address: pml4_table)
;   Set to address of page_directory_ptr_low + flags (0x03)
; - 0x1FF of the PML4. (Address: pml4_table + 0xFF8).
;   Set to address of page_directory_ptr_high + flags (0x03).
; - 0x000 of the low PDPT (Address: page_directory_ptr_low)
;   Set to the address of the page_directory + flags (0x03)
; - 0x1FC of the high PDPT (Address: page_directory_ptr_high + 0xFE0)
;   Set to address of page_directory + flags (0x03)
; - 0x000 of the PDT (Address: page_directory + 0x000)
;   Set to load address of kernel (1MB) rounded down to next MB (Zero) + flags
;   (0x83)

; In the end, the following virtual pages are mapped:
; - 2 MB page from 0x0. Mapped to physical 0x0+
; - 2 MB page from 0xFFFFFFFF00000000. Mapped to physical 0x0+
; And the following page table structures are created to support being able to
; use virtual address 0xFFFFFFFFFFFE0000 without needing to create new entries.
; - Entry 0x1FF of the high PDPT (address: page_directory_ptr_high + 0xFF8)
;   Set to address of mm_data_page_directory + flags (0x03)
; Entry 0x1FF of the mm_data_page_directory corresponds to address
; 0xFFFFFFFFFFFE0000.
; (This simplifies the memory manager startup code immensely)

; Fill in PML4 with the low PDPT table
mov rax, pml4_table
mov rbx, page_directory_ptr_low
add rbx, QWORD 0x0003
mov [rax], rbx

; Fill in PML4 with the high PDPT table
mov rax, pml4_table
add rax, 0x0FF8
mov rbx, page_directory_ptr_high
add rbx, QWORD 0x0003
mov [rax], rbx

; Fill in low PDPT
mov rax, page_directory_ptr_low
mov rbx, page_directory
add rbx, QWORD 0x03
mov [rax], rbx

; Fill in high PDPT
mov rax, page_directory_ptr_high
add rax, 0xFE0
mov rbx, page_directory
add rbx, QWORD 0x03
mov [rax], rbx

; Fill in PDT
mov rbx, QWORD 0x83
mov rax, page_directory
mov [rax], rbx

; Fill in memory manager PDT.
mov rax, page_directory_ptr_high
add rax, 0xFF8
mov rbx, mm_data_page_directory
add rbx, QWORD 0x03
mov [rax], rbx

; Fill in CR3 with the address of the PML4 table
mov rax, pml4_table
mov cr3, rax

; Enable paging
; (CR0.PG = 1, CR4.PAE = 1, and IA32_EFER.LME = 1)
mov rax, cr0
or eax, 0x80000000
mov cr0, rax

mov rax, cr4
or eax, 0x00000020
mov cr4, rax

; Assume IA32_EFER.LME is 1 already.

; Adjust stack pointer to point at upper memory.
mov rax, rsp
mov rbx, 0xFFFFFFFF00000000
or rax, rbx
mov rsp, rax

; Compute virtual address that can be edited to affect mapping of virtual
; address 0xFFFFFFFFFFFE0000.
EXTERN working_table_va_entry_addr
mov rax, mm_data_page_directory
add rax, QWORD 0xFF8
mov rbx, QWORD 0xFFFFFFFF00000000
or rax, rbx
mov rbx, working_table_va_entry_addr
mov [rbx], rax


; Jump in to the kernel proper. This indirect jump is necessary because of the
; way this part of the code is linked against lower memory, but is part of an
; image that's mostly linked for upper memory.
mov rax, main
jmp rax

align 4096
GLOBAL pml4_table
pml4_table:
  times 4096 db 0

; Kernel code page directories.
align 4096
page_directory_ptr_low:
  times 4096 db 0

align 4096
page_directory_ptr_high:
  times 4096 db 0

align 4096
page_directory:
  times 4096 db 0

; Kernel memory manager data page table entries.
align 4096
mm_data_page_directory:
  times 4096 db 0
