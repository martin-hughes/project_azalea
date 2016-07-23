[BITS 64]
SEGMENT .text

GLOBAL asm_proc_stop_this_proc
asm_proc_stop_this_proc:
	cli
	hlt

; Read the MSR specified by the first argument (RDI). Pass it back as a combined 64 bit result (RAX)
GLOBAL asm_proc_read_msr
asm_proc_read_msr:
  mov rcx, rdi
  rdmsr
  shl rdx, 32
  or rdx, rax
  mov rax, rdx
  ret

; Write the value specified by the second argument (RSI) into the MSR specified by RDI.
GLOBAL asm_proc_write_msr
asm_proc_write_msr:
  mov rax, rsi
  mov rdx, rsi
  shr rdx, 32
  mov rcx, rdi
  wrmsr
  ret
