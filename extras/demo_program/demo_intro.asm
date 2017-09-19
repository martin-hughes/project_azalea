[bits 64]
SEGMENT .pretext

EXTERN main
GLOBAL _start

_start:
  call main

loop:
  jmp loop
