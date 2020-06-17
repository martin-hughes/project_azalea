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
  mov edx, 0x001B0008

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
EXTERN syscall_pointers
EXTERN syscall_max_idx
asm_syscall_x64_syscall:
  ; Stop interrupts while we're fiddling with the stack, to avoid bad corruptions.
  cli

  ; Swap to this thread's kernel stack.
  swapgs
  mov [gs:16], rsp
  mov rsp, [gs:8]
  swapgs

  ; Save the registers that the calling convention says must be preserved
  push rbx
  push rbp
  push r12
  push r13
  push r14
  push r15

  sub rsp, 512
  fxsave64 [rsp]

  ; Save RCX (which is the stored RIP) and R11 (which is the stored RFLAGS).
  push rcx
  push r11

  sti

  ; Confirm that the requested system call is within the boundaries of the index table:
  mov r12, syscall_max_idx
  mov r12, [r12]

  cmp rax, r12
  ja invalid_syscall_idx

  ; The call index requested fits within the table. Extract the function's address and call it. Move R10 into RCX to
  ; fulfil the change between kernel interface ABI and x64 C function call ABI.
  mov r12, syscall_pointers
  shl rax, 3
  add rax, r12

  mov rcx, r10
  mov rbp, 0
  call [rax]

  jmp end_of_syscall

  invalid_syscall_idx:
  ; The requested index was too large. The error code for this is 2 - as defined in klib/misc/error_codes.h.
  mov rax, 2

  end_of_syscall:

  ; Put the RIP and R11 back again. Restore the FPU state.
  pop r11
  pop rcx

  fxrstor64 [rsp]
  add rsp, 512

  ; Stop interrupts, because otherwise they might get called with the user stack (which is bad)
  cli

  ; Restore the registers that must be preserved by the calling convention.
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbp
  pop rbx

  swapgs
  mov rsp, [gs:16]
  swapgs

  ; Carry on! Manually encode a "64-bit operands" (REX.W) prefix because:
  ; 1 - otherwise sysret thinks it's jumping back to 32-bit code, and hilarity ensues
  ; 2 - For some reason, adding the prefix using its mnemonic doesn't seem to work with NASM.
  db 0x48
  sysret

