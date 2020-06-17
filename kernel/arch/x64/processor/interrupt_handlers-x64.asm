[BITS 64]
SEGMENT .text

%include "kernel/arch/x64/processor/interrupts_defs.asm.h"

EXTERN proc_handle_interrupt

; Call the page fault handler.
GLOBAL asm_proc_page_fault_handler
EXTERN proc_page_fault_handler
asm_proc_page_fault_handler:
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
    mov rdi, [rsp + 128] ; Save error code
    mov rsi, cr2         ; Save fault address
    mov rdx, [rsp + 136] ; Save RIP
    mov rcx, [rsp + 160] ; Save RSP

    ALIGN_STACK_AND_SAVE
    mov r8, r12 ; Copy address of kernel stack as a parameter

    call proc_page_fault_handler

    RESTORE_ORIG_STACK

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
    iretq

; Define the IRQ handlers
EXTERN proc_handle_irq
GLOBAL asm_proc_handle_irq_0
asm_proc_handle_irq_0:
    DEF_IRQ_HANDLER 0, proc_handle_irq

GLOBAL asm_proc_handle_irq_1
asm_proc_handle_irq_1:
    DEF_IRQ_HANDLER 1, proc_handle_irq

GLOBAL asm_proc_handle_irq_2
asm_proc_handle_irq_2:
    DEF_IRQ_HANDLER 2, proc_handle_irq

GLOBAL asm_proc_handle_irq_3
asm_proc_handle_irq_3:
    DEF_IRQ_HANDLER 3, proc_handle_irq

GLOBAL asm_proc_handle_irq_4
asm_proc_handle_irq_4:
    DEF_IRQ_HANDLER 4, proc_handle_irq

GLOBAL asm_proc_handle_irq_5
asm_proc_handle_irq_5:
    DEF_IRQ_HANDLER 5, proc_handle_irq

GLOBAL asm_proc_handle_irq_6
asm_proc_handle_irq_6:
    DEF_IRQ_HANDLER 6, proc_handle_irq

GLOBAL asm_proc_handle_irq_7
asm_proc_handle_irq_7:
    DEF_IRQ_HANDLER 7, proc_handle_irq

GLOBAL asm_proc_handle_irq_8
asm_proc_handle_irq_8:
    DEF_IRQ_HANDLER 8, proc_handle_irq

GLOBAL asm_proc_handle_irq_9
asm_proc_handle_irq_9:
    DEF_IRQ_HANDLER 9, proc_handle_irq

GLOBAL asm_proc_handle_irq_10
asm_proc_handle_irq_10:
    DEF_IRQ_HANDLER 10, proc_handle_irq

GLOBAL asm_proc_handle_irq_11
asm_proc_handle_irq_11:
    DEF_IRQ_HANDLER 11, proc_handle_irq

GLOBAL asm_proc_handle_irq_12
asm_proc_handle_irq_12:
    DEF_IRQ_HANDLER 12, proc_handle_irq

GLOBAL asm_proc_handle_irq_13
asm_proc_handle_irq_13:
    DEF_IRQ_HANDLER 13, proc_handle_irq

GLOBAL asm_proc_handle_irq_14
asm_proc_handle_irq_14:
    DEF_IRQ_HANDLER 14, proc_handle_irq

GLOBAL asm_proc_handle_irq_15
asm_proc_handle_irq_15:
    DEF_IRQ_HANDLER 15, proc_handle_irq

; Define the exception handlers:

; Exception 0
GLOBAL asm_proc_div_by_zero_fault_handler
EXTERN proc_div_by_zero_fault_handler
asm_proc_div_by_zero_fault_handler:
    DEF_EXCEPTION_HANDLER proc_div_by_zero_fault_handler, 0

; Exception 1
GLOBAL asm_proc_debug_fault_handler
EXTERN proc_debug_fault_handler
asm_proc_debug_fault_handler:
    DEF_EXCEPTION_HANDLER proc_debug_fault_handler, 0

; Exception 2 - NMIs are used for inter-processor control messages.
GLOBAL asm_proc_nmi_int_handler
EXTERN proc_mp_x64_receive_signal_int
asm_proc_nmi_int_handler:
    DEF_EXCEPTION_HANDLER proc_mp_x64_receive_signal_int, 0

; Exception 3
GLOBAL asm_proc_brkpt_trap_handler
EXTERN proc_brkpt_trap_handler
asm_proc_brkpt_trap_handler:
    DEF_EXCEPTION_HANDLER proc_brkpt_trap_handler, 0

; Exception 4
GLOBAL asm_proc_overflow_trap_handler
EXTERN proc_overflow_trap_handler
asm_proc_overflow_trap_handler:
    DEF_EXCEPTION_HANDLER proc_overflow_trap_handler, 0

; Exception 5
GLOBAL asm_proc_bound_range_fault_handler
EXTERN proc_bound_range_fault_handler
asm_proc_bound_range_fault_handler:
    DEF_EXCEPTION_HANDLER proc_bound_range_fault_handler, 0

; Exception 6
GLOBAL asm_proc_invalid_opcode_fault_handler
EXTERN proc_invalid_opcode_fault_handler
asm_proc_invalid_opcode_fault_handler:
    DEF_EXCEPTION_HANDLER proc_invalid_opcode_fault_handler, 0

; Exception 7
GLOBAL asm_proc_device_not_avail_fault_handler
EXTERN proc_device_not_avail_fault_handler
asm_proc_device_not_avail_fault_handler:
    DEF_EXCEPTION_HANDLER proc_device_not_avail_fault_handler, 0

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
    DEF_EXCEPTION_HANDLER proc_fp_except_fault_handler, 0

; Exception 17
GLOBAL asm_proc_align_check_fault_handler
EXTERN proc_align_check_fault_handler
asm_proc_align_check_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_align_check_fault_handler

; Exception 18
GLOBAL asm_proc_machine_check_abort_handler
EXTERN proc_machine_check_abort_handler
asm_proc_machine_check_abort_handler:
    DEF_EXCEPTION_HANDLER proc_machine_check_abort_handler, 0

; Exception 19
GLOBAL asm_proc_simd_fpe_fault_handler
EXTERN proc_simd_fpe_fault_handler
asm_proc_simd_fpe_fault_handler:
    DEF_EXCEPTION_HANDLER proc_simd_fpe_fault_handler, 0


; Exception 20
GLOBAL asm_proc_virt_except_fault_handler
EXTERN proc_virt_except_fault_handler
asm_proc_virt_except_fault_handler:
    DEF_EXCEPTION_HANDLER proc_virt_except_fault_handler, 0

; Exception 30
GLOBAL asm_proc_security_fault_handler
EXTERN proc_security_fault_handler
asm_proc_security_fault_handler:
    DEF_ERR_CODE_INT_HANDLER proc_security_fault_handler

; Define fallback handlers for all other interrupts

%assign i 0
%rep    256

GLOBAL asm_proc_interrupt_%[i]_handler
asm_proc_interrupt_%[i]_handler:
    DEF_INT_HANDLER proc_handle_interrupt, i

%assign i i+1
%endrep
