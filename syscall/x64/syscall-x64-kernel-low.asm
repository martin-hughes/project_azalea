[BITS 64]
SEGMENT .text

; The CPU-level part of the kernel's x64 system call interface. It is entirely based upon the syscall/sysret
; instructions.

; Enable the kernel's system call interface.
GLOBAL asm_syscall_x64_prepare
asm_syscall_x64_prepare:
  ; Things to do:
  ; 1: Set the SCE bit to enabled.
  ; 2: Load the default kernel mode CS and SS into the appropriate MSR.
  ; 3: Load the default user mode CS and SS into the appropriate MSR.
  ; 4: Load the syscall instruction pointer into the appropriate MSR.

  ; 1: Set the SCE bit to enabled. It is bit 0 of EFER (0xC0000080)
  mov ecx, 0xC0000080
  rdmsr ; Stores EFER in EDX:EAX
  or eax, 0x00000001
  wrmsr

  ; 2: Load the default CS and SS into the appropriate MSRs.
  ; Both go into bits 47-32 of IA32_STAR (0xC0000081). SS is then assumed to be one segment descriptor along from CS.
  ; 3: Similar to #2, load the default user mode CS and SS. These go into bits 63:48 (which is the highest 16 bits of
  ; ECX)
  mov ecx, 0xC0000081
  rdmsr

  ; Now load in the two CSs - just use a magic number for the time being.
  mov edx, 0x00180008

  ; Put all those segments back.
  wrmsr

  ; 4: Load the syscall instruction pointer into the approriate MSR.
  ; The appropriate MSR is IA32_LSTAR.
  mov ecx, 0xC0000082 ; IA32_LSTAR
  rdmsr

  ; Remember that WRMSR takes the high order 32 bits from EDX, and the low order 32 bits from RAX.
  mov rax, asm_syscall_x64_syscall
  mov rdx, rax
  shr rdx, 32
  wrmsr

  ret

; The system call interface!
; There is no guarantee of any register remaining unchanged.
; System call number comes by RAX.
GLOBAL asm_syscall_x64_syscall
EXTERN syscall_x64_kernel_syscall
EXTERN syscall_x64_kernel_stack
asm_syscall_x64_syscall:
  ; Switch to kernel stack, saving the user mode one.
  mov rbx, rsp
  mov rdx, syscall_x64_kernel_stack
  mov rdx, [rdx]
  mov rsp, rdx

  ; Save RCX (which is the stored RIP) and R11 (which is the stored RFLAGS), as well as RBX which is the address of the
  ; user-mode stack.
  push rbx
  push rcx
  push r11

  ; Do more stuff than this! Like set up the arguments and stuff.
  mov rbx, syscall_x64_kernel_syscall
  call rbx

  ; Put the RIP and R11 back again.
  pop r11
  pop rcx

  ; Stop interrupts, because otherwise they might get called with the user stack (which is bad)
  cli

  ; Restore the stack.
  pop rbx
  mov rsp, rbx

  ; Carry on! Manually encode a "64-bit operands" (REX.W) prefix because:
  ; 1 - otherwise sysret thinks it's jumping back to 32-bit code, and hilarity ensues
  ; 2 - For some reason, adding the prefix using it's mnemonic doesn't seem to work with NASM.
  db 0x48
  sysret

