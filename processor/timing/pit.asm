[BITS 64]
SEGMENT .text

; TODO: This code is currently unused - remove

%macro OUTB 2
  mov al, %2
  out %1, al
%endmacro

global asm_proc_init_pit
asm_proc_init_pit:
  ; PIT initialisation
  OUTB 0x43, 0x36
  OUTB 0x40, 0x9B
  OUTB 0x40, 0x2E
  ret
