[BITS 64]

beginning:
  syscall
  mov rax, 2
  jmp beginning
