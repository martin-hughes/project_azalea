[bits 64]
SEGMENT .pretext

EXTERN main
GLOBAL pre_main

pre_main:
  mov rsp, 0x2f0000
  call main

loop:
  hlt
  jmp loop
