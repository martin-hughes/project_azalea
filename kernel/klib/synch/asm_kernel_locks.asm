[BITS 64]

; RDI will contain the address of the lock to lock.
GLOBAL asm_klib_synch_spinlock_lock
asm_klib_synch_spinlock_lock:
  mov rax, 0
  mov rdx, 0x01
  lock cmpxchg [rdi], rdx

  cmp rax, 1
  je asm_klib_synch_spinlock_lock

  ret

GLOBAL asm_klib_synch_spinlock_try_lock
asm_klib_synch_spinlock_try_lock:
  mov rax, 0
  mov rdx, 0x01
  lock cmpxchg [rdi], rdx

  cmp rax, 1
  je try_lock_failed
    mov rax, 1
    jmp try_lock_finish
  try_lock_failed:
    mov rax, 0
  try_lock_finish:
  ret
