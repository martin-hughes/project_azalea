; Application Processor (AP) startup code - 64-bit part.
;
[BITS 64]
SEGMENT .pretext

EXTERN proc_mp_ap_startup
GLOBAL asm_proc_mp_ap_startup
asm_proc_mp_ap_startup:
  ; Set up a suitable stack, then call the main code.
  mov rax, temp_64_bit_stack_end
  mov rsp, rax

  mov rax, proc_mp_ap_startup
  call rax
  cli
  hlt

ALIGN 8
temp_64_bit_stack:
  ; This only needs to be small - it is needed for as long as it takes to finish proc_mp_ap_startup, since after that
  ; the stack will either be the one in use when an interrupt hits, or the one given by the TR.

  times 4096 db 0

temp_64_bit_stack_end:
  times 8 db 0
