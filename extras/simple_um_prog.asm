[BITS 64]
ORG 0x200000

; Set up a stack in a space we know to be mapped.
mov rax, 0x2F0000
mov rsp, rax

beginning:
  ; Make a couple of successful system calls (these output the string at the bottom)
  mov rax, 0
  mov rdi, string
  mov rsi, 15
  syscall

  mov rax, 0
  mov rdi, string
  mov rsi, 15
  syscall

  ; Make a system call with an invalid index.
  mov rax, 1
  syscall

  ; If the result is 2 (invalid system call index) - which it should be then jump to "panic".
  cmp rax, 2
  je panic

  ; Otherwise, just start again.
  jmp beginning

panic:
  ; Force a panic (int 49 does this at the moment, it won't in future!)
  int 49
  jmp beginning

string:
db 'Hello everyone',10,0
