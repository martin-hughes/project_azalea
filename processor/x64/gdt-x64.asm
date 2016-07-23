[BITS 64]
SEGMENT .text

; Load the GDT, then do a "far jump" to pick up the correct segment.  This
; should leave the stack unaffected, so just return from there. Fill in non-code
; segments afterwards.
GLOBAL asm_proc_load_gdt
asm_proc_load_gdt:
  ; Fill in the GDT.
  mov rax, gdt_pointer
  lgdt [rax]

  ; x64 mode doesn't allow for jumps to set the CS register, so fill in in via
  ; a simulated interrupt return.
  mov rax, 0x00000010
  push rax        ; Load SS on to the stack
  push rsp
  pushf           ; Push EFLAGS on to the stack
  mov rax, 0x00000008
  push rax        ; Push CS on to the stack
  mov rax, _lgdt_jump
  push rax        ; Push RIP on to the stack.
  iretq           ; Bounce over to the next instruction.

_lgdt_jump:

  ; Fill in the remaining segments. A lot of these are ignored in 64-bit mode,
  ; but it does no harm...
  mov ax, 0x10      ; 0x10 is the offset in the GDT to our data segment
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; Because we pushed RSP after adding RAX to the stack, the stack that IRET
  ; has restored still has RAX on it. Pop it out.
  pop rax

  ; The stack should now be good to use again.
  ret

GLOBAL asm_proc_load_tss
asm_proc_load_tss:
  push rax
  mov ax, 48
  ltr ax
  pop rax
  ret

; This is the table that the processor uses to locate the GDT proper.
GLOBAL gdt_pointer
gdt_pointer:
  ; Length of the table, in bytes, minus 1
  dw end_of_gdt_table - gdt_table - 1
  dq gdt_table

; A helper for filling in GDT entries.
;
; Since this is 64-bit mode, it's safe to set both the "base" and "limit"
; entries to 0, so this macro skips them. Also, it's assumed that all code runs
; in 64-bit mode.
;
; Argument is is the privilege level which must be between 0 and 3, inclusive.
; The second parameter is a type which must be one of the 4-bit combinations
; given in the intel documentation for the "Segment Descriptor Types" field.
%macro GDT_ENTRY 2
  dw 0x0000 ; Low-order two bytes of the "limit" field.
  dw 0x0000 ; Low-order two bytes of the "base" field.
  db 0x00 ; Third byte of the "base" field, which is unused.
  db (0x90 + (%1 * 32) + %2) ; First bit is "Present", followed by the
          ; two-bit privilege level, followed by "System." The final 4 bits are
          ; the 4-bit type described above.
  db 0x20 ; Only the "64-bit code" field is set. The second nybble part of
          ; "limit" is unused.
  db 0x00 ; Top byte of the "base" field
%endmacro

; A helper for filling in the TSS descriptor within the GDT.
;
; Since this is x64 code, should only be used once. It is filled in later, once the paging / memory system is operative.
%macro TSS_ENTRY 0
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000
%endmacro

; These are the two type values used by the kernel
%define RW_DATA_SEG 2
%define RO_CODE_SEG 10

; The complete GDT table. It consists of a NULL entry, followed by Kernel and
; User space components. It should never change in normal operation.
ALIGN 8
gdt_table:
  ; NULL entry
  dw 0x0000
  dw 0x0000
  dw 0x0000
  dw 0x0000

  ; Kernel code entry.
  GDT_ENTRY 0, RO_CODE_SEG

  ; Kernel data entry (including stack).
  GDT_ENTRY 0, RW_DATA_SEG

  ; User code entry.
  GDT_ENTRY 3, RO_CODE_SEG

  ; User data entry.
  GDT_ENTRY 3, RW_DATA_SEG

  ; Another user code entry, to make the syscall/sysret easier. For some reason, sysret looks at
  ; the index + 16 for a CS, and index + 8 for an SS.
  GDT_ENTRY 3, RO_CODE_SEG

  ; TSS descriptor entry
GLOBAL tss_gdt_entry
tss_gdt_entry:
  TSS_ENTRY

end_of_gdt_table:

