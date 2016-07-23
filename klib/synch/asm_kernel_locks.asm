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
