[BITS 64]
SEGMENT .text

EXTERN end_of_irq_ack_fn

; Stops interrupts on this processor.
GLOBAL asm_proc_stop_interrupts
asm_proc_stop_interrupts:
	cli
	ret

; Starts interrupt handling on this processor.
; TODO: Is it necessary to deal with missing IRQs here?
GLOBAL asm_proc_start_interrupts
asm_proc_start_interrupts:
    sti
    push rax
    mov rax, end_of_irq_ack_fn
    call [rax]
    pop rax
    ret

GLOBAL asm_proc_install_idt
extern interrupt_descriptor_table
idt_pointer:
  ; Length of the table, in bytes.
  dw (256 * 16) - 1
  dq interrupt_descriptor_table

asm_proc_install_idt:
    push rax
    mov rax, idt_pointer
    lidt [rax]

    mov rax, end_of_irq_ack_fn
    call [rax]

    pop rax

    ret

%macro OUTB 2
    mov al, %2
    out %1, al
%endmacro

; A default handler for interrupts. Simply calls a named function with no
; arguments. Sadly, it seems that it is necessary to push all registers
; manually. Ignore the SSE-type registers - these are unused in the kernel, so
; we can deal with them separately at task-switching time.
;
; TODO: Make sure this happens.
; TODO: Similarly for the error code handler, below.
%macro DEF_INT_HANDLER 1
    cli
    pushf
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    call %1
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popf
    sti
    iretq
%endmacro

%macro DEF_ERR_CODE_INT_HANDLER 1
    cli
    pushf
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, [rsp + 128]
    mov rsi, [rsp + 136]
    call %1
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popf
    sti
    iretq
%endmacro

; A default interrupt handler.
GLOBAL asm_proc_def_interrupt_handler
EXTERN proc_def_interrupt_handler
asm_proc_def_interrupt_handler:
    DEF_INT_HANDLER proc_def_interrupt_handler

GLOBAL asm_proc_def_irq_handler
asm_proc_def_irq_handler:
  DEF_INT_HANDLER end_of_irq_ack_fn_wrapper

; Call the page fault handler.
GLOBAL asm_proc_page_fault_handler
EXTERN proc_page_fault_handler
asm_proc_page_fault_handler:
    cli
    pushf
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, [rsp + 128]
    mov rsi, cr2
    mov rdx, [rsp + 136]
    call proc_page_fault_handler
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    popf
    add rsp, 8
    sti
    iretq


; Define the exception handlers:

; Exception 0
GLOBAL asm_proc_div_by_zero_fault_handler
EXTERN proc_div_by_zero_fault_handler
asm_proc_div_by_zero_fault_handler:
    DEF_INT_HANDLER proc_div_by_zero_fault_handler

; Exception 1
GLOBAL asm_proc_debug_fault_handler
EXTERN proc_debug_fault_handler
asm_proc_debug_fault_handler:
    DEF_INT_HANDLER proc_debug_fault_handler

; Exception 2 - NMIs are used for inter-processor control messages.
GLOBAL asm_proc_nmi_int_handler
EXTERN proc_mp_x64_receive_signal_int
asm_proc_nmi_int_handler:
    DEF_INT_HANDLER proc_mp_x64_receive_signal_int

; Exception 3
GLOBAL asm_proc_brkpt_trap_handler
EXTERN proc_brkpt_trap_handler
asm_proc_brkpt_trap_handler:
    DEF_INT_HANDLER proc_brkpt_trap_handler

; Exception 4
GLOBAL asm_proc_overflow_trap_handler
EXTERN proc_overflow_trap_handler
asm_proc_overflow_trap_handler:
    DEF_INT_HANDLER proc_overflow_trap_handler

; Exception 5
GLOBAL asm_proc_bound_range_fault_handler
EXTERN proc_bound_range_fault_handler
asm_proc_bound_range_fault_handler:
    DEF_INT_HANDLER proc_bound_range_fault_handler

; Exception 6
GLOBAL asm_proc_invalid_opcode_fault_handler
EXTERN proc_invalid_opcode_fault_handler
asm_proc_invalid_opcode_fault_handler:
    DEF_INT_HANDLER proc_invalid_opcode_fault_handler

; Exception 7
GLOBAL asm_proc_device_not_avail_fault_handler
EXTERN proc_device_not_avail_fault_handler
asm_proc_device_not_avail_fault_handler:
    DEF_INT_HANDLER proc_device_not_avail_fault_handler

; Exception 8
GLOBAL asm_proc_double_fault_abort_handler
EXTERN proc_double_fault_abort_handler
asm_proc_double_fault_abort_handler:
    DEF_ERR_CODE_INT_HANDLER proc_double_fault_abort_handler

; Exception 10
GLOBAL asm_proc_invalid_tss_fault_handler
EXTERN proc_invalid_tss_fault_handler
asm_proc_invalid_tss_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_invalid_tss_fault_handler

; Exception 11
GLOBAL asm_proc_seg_not_present_fault_handler
EXTERN proc_seg_not_present_fault_handler
asm_proc_seg_not_present_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_seg_not_present_fault_handler

; Exception 12
GLOBAL asm_proc_ss_fault_handler
EXTERN proc_ss_fault_handler
asm_proc_ss_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_ss_fault_handler

; Exception 13
GLOBAL asm_proc_gen_prot_fault_handler
EXTERN proc_gen_prot_fault_handler
asm_proc_gen_prot_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_gen_prot_fault_handler

; Exception 16
GLOBAL asm_proc_fp_except_fault_handler
EXTERN proc_fp_except_fault_handler
asm_proc_fp_except_fault_handler:
    DEF_INT_HANDLER proc_fp_except_fault_handler

; Exception 17
GLOBAL asm_proc_align_check_fault_handler
EXTERN proc_align_check_fault_handler
asm_proc_align_check_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_align_check_fault_handler

; Exception 18
GLOBAL asm_proc_machine_check_abort_handler
EXTERN proc_machine_check_abort_handler
asm_proc_machine_check_abort_handler:
    DEF_INT_HANDLER proc_machine_check_abort_handler

; Exception 19
GLOBAL asm_proc_simd_fpe_fault_handler
EXTERN proc_simd_fpe_fault_handler
asm_proc_simd_fpe_fault_handler:
    DEF_INT_HANDLER proc_simd_fpe_fault_handler


; Exception 20
GLOBAL asm_proc_virt_except_fault_handler
EXTERN proc_virt_except_fault_handler
asm_proc_virt_except_fault_handler:
    DEF_INT_HANDLER proc_virt_except_fault_handler

; Exception 30
GLOBAL asm_proc_security_fault_handler
EXTERN proc_security_fault_handler
asm_proc_security_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_security_fault_handler

end_of_irq_ack_fn_wrapper:
  mov rax, end_of_irq_ack_fn
  call [rax]
  ret
