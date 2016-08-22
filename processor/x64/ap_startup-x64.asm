; Application Processor (AP) startup code.
;
; The kernel gains control of the APs by redirecting their NMI handling code to asm_proc_pure64_new_nmi_handler. This
; is necessary because Pure64 leaves them halted with interrupts disabled, and it's too much effort to go through a
; full INIT and STARTUP IPI cycle.

[BITS 64]
SEGMENT .text

; Once an NMI triggers on the AP, it eventually calls back to here, via asm_proc_pure64_new_nmi_handler. We can then
; call back out to proc_mp_ap_startup to continue in C++.
EXTERN proc_mp_ap_startup
asm_proc_mp_ap_startup:
  call proc_mp_ap_startup
  int 12

; This code is copied over the Pure64 NMI handling code, to direct it to our AP startup code, below.
GLOBAL asm_proc_pure64_nmi_trampoline_start
GLOBAL asm_proc_pure64_nmi_trampoline_end
asm_proc_pure64_nmi_trampoline_start:
  mov rax, asm_proc_pure64_new_nmi_handler
  jmp rax
asm_proc_pure64_nmi_trampoline_end:

SEGMENT .pretext

; Called in response to an NMI on the APs after asm_proc_hack_pure64_nmi has been triggered.
; Do a minimal amount of setup, then iret back to asm_proc_mp_ap_startup.
EXTERN pml4_table
asm_proc_pure64_new_nmi_handler:
  ; Load a decent set of page tables
  mov rax, pml4_table
  mov cr3, rax

  ; Get out of being in an interrupt handler with a bit of hackery
  mov rax, ss
  push rax        ; Load SS on to the stack
  push rsp
  pushf           ; Push EFLAGS on to the stack
  mov rax, cs
  push rax        ; Push CS on to the stack
  mov rax, _breakout_jump
  push rax        ; Push RIP on to the stack.

  iretq           ; Bounce over to the next instruction. This allows the next NMI to be generated when needed.

_breakout_jump:

  ; Because we pushed RSP after adding RAX to the stack, the stack that IRET
  ; has restored still has RAX on it. Pop it out.
  pop rax

  ; The stack should now be good to use again.
  mov rax, asm_proc_mp_ap_startup
  jmp rax
