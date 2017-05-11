[bits 64]
SEGMENT .pretext

EXTERN main
GLOBAL pre_main

pre_main:
  call main

loop:
  hlt
  jmp loop
