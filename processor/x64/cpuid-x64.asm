[BITS 64]
SEGMENT .text

; Reads CPUID for the requested fields. Parameters are in the following order:
; 1 - RDI - The value to be passed to CPUID in EAX
; 2 - RSI - The value to be passed to CPUID in ECX (should normally be 0)
; 3 - RDX - A pointer to a 64 bit space that will contain the results of EBX:EAX (i.e. EBX is in the higher-order 32 bits)
; 4 - RCX - A pointer to a 64 bit space that will contain the results of EDX:ECX
;
; It is assumed throughout that CPUID is supported (reasonable, since the system is in 64-bit mode)
GLOBAL asm_proc_read_cpuid
asm_proc_read_cpuid:
  ; Save the output locations for later
  mov r8, rdx
  mov r9, rcx

  ; Set up for CPUID
  mov rax, rdi
  mov rcx, rsi
  cpuid

  mov r10, 0x00000000FFFFFFFF

  ; Save EBX:EAX
  shl rbx, 32
  and rax, r10
  or rbx, rax
  mov [r8], rbx

  ;Save EDX:ECX
  shl rdx, 32
  and rcx, r10
  or rdx, rcx
  mov [r9], rdx

  ret
