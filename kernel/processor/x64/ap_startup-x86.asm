; AP startup code - 32-bit portion.

[BITS 32]
SEGMENT .pretext

EXTERN pml4_table
EXTERN asm_proc_mp_ap_startup

GLOBAL asm_ap_x86_entry
asm_ap_x86_entry:
  ; Disable paging:
  mov eax, cr0
  and eax, 0x7FFFFFFF
  mov cr0, eax

  ; Reset segment selectors
  mov ax, 0x10
  mov ss, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; Enable PAE:
  mov eax, cr4
  or eax, 0x00000020
  mov cr4, eax

  ; Fill in CR3 with the address of the PML4 table - we re-use the PML4 that the BSP used during startup - it'll do
  ; for now.
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

  mov eax, gdt_32_bit_ptr
  lgdt [eax]

  ; Jump to long mode code
  jmp 8:asm_proc_mp_ap_startup

ALIGN 8
gdt_32_bit:
  dq 0x0000000000000000 ; Null descriptor
  dq 0x00209A0000000000 ; 64-bit code descriptor (exec/read).
  dq 0x0000920000000000 ; 64-bit data descriptor (read/write).

  gdt_32_bit_ptr:
  dw 23 ; 16-bit Size (Limit) of GDT.
  dd gdt_32_bit ; Address of actual GDT
