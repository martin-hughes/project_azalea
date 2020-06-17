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

; Read the specified processor port.
; Parameter 1 (RDI): port ID
; Parameter 2 (RSI): bits of width to use. It is assumed that the value is 8, 16 or 32. Undefined results otherwise.
GLOBAL proc_read_port
proc_read_port:
  mov rax, 0
  mov dx, di

  cmp rsi, 8
  jne aprp_try_16
    in al, dx
    jmp aprp_done

  aprp_try_16:
  cmp rsi, 16
  jne aprp_try_32
    in ax, dx
    jmp aprp_done

  aprp_try_32:
  cmp rsi, 32
  jne aprp_done
    in eax, dx

  aprp_done:
  ret


; Write to the specified processor port
; Parameter 1 (RDI) Port ID
; Parameter 2 (RSI) Value to write
; Parameter 3 (RDX) bits of width to use. It is assumed that the value is 8, 16 or 32. Undefined results otherwise.
GLOBAL proc_write_port
proc_write_port:
  ; Swap parameters into useable registers. RAX is the value being written. RDX will be the port to write to.
  mov r8, rdx
  mov rdx, 0
  mov dx, di
  mov rax, rsi

  cmp r8, 8
  jne apwp_try_16
    out dx, al
    jmp apwp_done

  apwp_try_16:
  cmp r8, 16
  jne apwp_try_32
    out dx, ax
    jmp apwp_done

  apwp_try_32:
  cmp r8, 32
  jne apwp_done
    out dx, eax

  apwp_done:
  ret

GLOBAL asm_proc_enable_fp_math
asm_proc_enable_fp_math:
  ; Disable emulation (i.e. use the real floating point unit) and set monitor flag on.
  mov rax, cr0
  bts rax, 1
  btr rax, 2
  mov cr0, rax

  ; Enable SSE intructions and SIMD floating point exceptions.
  mov rax, cr4
  bts rax, 9
  bts rax, 10
  mov cr4, rax

  finit

  ret
