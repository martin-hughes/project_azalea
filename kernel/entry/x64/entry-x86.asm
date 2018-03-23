[BITS 32]
SEGMENT .pretext_32
EXTERN pre_main_64
GLOBAL pre_main_32

; Control is passed to the x64 kernel via GRUB, which gives us 32-bit mode. After GRUB, execution starts at
; pre_main_32. We quickly jump to 64-bit mode, then pass to pre_main_64, which does some early setup before jumping to
; the C code in main().

; Just in case some muppet tries to execute us directly, just loop
muppet_check:
  mov ax, 0
  jmp muppet_check

; Multiboot headers:
align 8
multiboot_header_start:
  dd 0x1BADB002 ; Magic number
  dd 0x00010003 ; Flags requesting 4kb alignment, memory information, and to use the remainder of the header.
  dd 0xE4514FFB ; Checksum
  dd multiboot_header_start ; Start of multiboot header.
  dd 0x00100000 ; Physical load address
  dd 0 ; Physical load end address
  dd 0 ; BSS end address (but we don't use GRUB's BSS zeroing ability, so just set to 0.
  dd pre_main_32 ; Entry point
  dd 0 ; Requested display mode (not used)
  dd 0 ; Requested display width (not used)
  dd 0 ; Requested display height (not used)
  dd 0 ; Requested display BPP (not used)
multiboot_header_end:

pre_main_32:
  ; Spend the most minimal time here. Disable paging (although it shouldn't really be enabled), configure PAE and LME,
  ; setup a trivial GDT and jump to 64-bit mode. Note the similarity between this code and asm_ap_x86_entry

  ; Save multiboot details in registers that cause them to be sent as parameters to main() later on.
  mov edi, eax
  mov esi, ebx

  ; Disable paging:
  mov eax, cr0
  and eax, 0x7FFFFFFF
  mov cr0, eax

  ; Enable PAE:
  mov eax, cr4
  or eax, 0x00000020
  mov cr4, eax

  ; Configure a set of page tables:
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
  mov eax, pml4_table
  mov ebx, page_directory_ptr_low
  add ebx, DWORD 0x0003
  mov [eax], ebx

  ; Fill in PML4 with the high PDPT table
  mov eax, pml4_table
  add eax, 0x0FF8
  mov ebx, page_directory_ptr_high
  add ebx, DWORD 0x0003
  mov [eax], ebx

  ; Fill in low PDPT
  mov eax, page_directory_ptr_low
  mov ebx, page_directory
  add ebx, DWORD 0x03
  mov [eax], ebx

  ; Fill in high PDPT
  mov eax, page_directory_ptr_high
  add eax, 0xFE0
  mov ebx, page_directory
  add ebx, DWORD 0x03
  mov [eax], ebx

  ; Fill in PDT
  mov ebx, DWORD 0x83
  mov eax, page_directory
  mov [eax], ebx

  ; Fill in memory manager PDT.
  mov eax, page_directory_ptr_high
  add eax, 0xFF8
  mov ebx, mm_data_page_directory
  add ebx, DWORD 0x03
  mov [eax], ebx

  ; Fill in CR3 with the address of the PML4 table
  mov eax, pml4_table
  mov cr3, eax

  ; Enable LME:
  mov ecx, 0xC0000080 ; IA32_EFER
  rdmsr
  or eax, 0x00000100
  wrmsr

  ; Enable paging
  ; (CR0.PG = 1, CR4.PAE = 1, and IA32_EFER.LME = 1)
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; Setup trivial GDT
  mov eax, gdt_32_bit_ptr
  lgdt [eax]

  ; Jump to long mode code
  jmp 8:pre_main_64

ALIGN 8
gdt_32_bit:
  dq 0x0000000000000000 ; Null descriptor
  dq 0x00209A0000000000 ; 64-bit code descriptor (exec/read).
  dq 0x0000920000000000 ; 64-bit data descriptor (read/write).

gdt_32_bit_ptr:
  dw 23 ; 16-bit Size (Limit) of GDT.
  dd gdt_32_bit ; Address of actual GDT

align 4096
GLOBAL pml4_table
pml4_table:
  times 4096 db 0

; Kernel code page directories.
align 4096
;GLOBAL page_directory_ptr_low
page_directory_ptr_low:
  times 4096 db 0

align 4096
;GLOBAL page_directory_ptr_high
page_directory_ptr_high:
  times 4096 db 0

align 4096
;GLOBAL page_directory
page_directory:
  times 4096 db 0

; Kernel memory manager data page table entries.
align 4096
GLOBAL mm_data_page_directory
mm_data_page_directory:
  times 4096 db 0
