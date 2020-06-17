; Application Processor (AP) startup code - 64-bit part.
;
[BITS 64]
SEGMENT .pretext

EXTERN proc_mp_ap_startup
GLOBAL asm_proc_mp_ap_startup
asm_proc_mp_ap_startup:
  ; Set up a suitable stack, then call the main code. Remember that this is linked lower-half, but the main kernel code
  ; is linked higher half, so this value will not be accessible after the lower-half is unmapped.
  mov rax, [asm_next_startup_stack]
  mov rsp, rax

  mov rbp, 0

  mov rax, proc_mp_ap_startup
  call rax
return_pt:
  cli
  hlt
  jmp return_pt

ALIGN 16
GLOBAL asm_next_startup_stack
asm_next_startup_stack:
  times 8 db 0
