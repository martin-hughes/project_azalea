; Real mode entry point for AP startup.

[BITS 16]
SEGMENT .pretext_16

GLOBAL asm_ap_trampoline_start
GLOBAL asm_ap_trampoline_end
GLOBAL temp_32_bit_gdt_ptr
EXTERN asm_ap_x86_entry

; The code between asm_ap_trampoline_start and _end is copied to a suitable place in lower memory to be used as a
; startup vector in response to the processor receiving a STARTUP IPI message. It configures a trivial 32-bit GDT and
; jumps to 32-bit code ASAP.
asm_ap_trampoline_start:

; There's no reason why CS should be anything troublesome, but just in case.
  jmp 0:dword begin_set_A20

; Enable the A20 gate via the keyboard controller. If this doesn't work then the system will time out after a short
; time, which is fine.
begin_set_A20:

  in al, 0x64
  test al, 0x02
  jnz begin_set_A20
  mov al, 0xD1
  out 0x64, al

check_A20:
  in al, 0x64
  test al, 0x02
  jnz check_A20
  mov al, 0xDF
  out 0x60, al

  ; Load a temporary GDT to get us in to protected mode.
  mov eax, dword temp_32_bit_gdt_ptr
  lgdt [CS:eax]

  mov ebx, cr0
  bts ebx, 0
  mov cr0, ebx

  jmp 8:dword ap_in_32_bit

[BITS 32]
ap_in_32_bit:
  mov eax, dword asm_ap_x86_entry
  jmp eax

ALIGN 16
temp_32_bit_gdt_ptr:
  dw temp_32_bit_gdt_end - temp_32_bit_gdt_start - 1
  dq temp_32_bit_gdt_start

ALIGN 16
temp_32_bit_gdt_start:
  dw 0x0000, 0x0000, 0x0000, 0x0000 ; NULL entry
  dw 0xFFFF, 0x0000, 0x9A00, 0x00CF ; Code entry
  dw 0xFFFF, 0x0000, 0x9200, 0x00CF ; Data entry
temp_32_bit_gdt_end:

asm_ap_trampoline_end:
  dw 0x0000
