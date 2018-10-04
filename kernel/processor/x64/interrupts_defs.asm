%ifndef X64_INTERRUPT_DEFS
%define X64_INTERRUPT_DEFS

EXTERN end_of_irq_ack_fn

GLOBAL end_of_irq_ack_fn_wrapper
end_of_irq_ack_fn_wrapper:
  mov rax, end_of_irq_ack_fn
  call [rax]
  ret


; Move the stack pointer sufficiently to save the FPU state. Make sure that the stack pointer is aligned on a 16-byte
; boundary before and after this, so that the C code can assume a 16-byte stack alignment. Save the original,
; pre-FPU-save stack pointer in R12, since this must be preserved by C-code.
%macro ALIGN_STACK_AND_SAVE 0
  mov r12, rsp
  sub rsp, 512
  and rsp, qword 0xFFFFFFFFFFFFFFF0
  fxsave64 [rsp]
  sub rsp, 16
%endmacro

; Restore the stack to it's pre-alignment state, since it can be arbitrarily aligned when an interrupt is called.
%macro RESTORE_ORIG_STACK 0
  add rsp, 16
  fxrstor64 [rsp]
  mov rsp, r12
%endmacro

; A default handler for interrupts. Simply calls a named function with the numerical argument given.
%macro DEF_INT_HANDLER 2
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

    ALIGN_STACK_AND_SAVE

    mov rdi, %2
    call %1

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

    ALIGN_STACK_AND_SAVE

    call %1

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
    sti
    iretq
%endmacro

%macro DEF_IRQ_HANDLER 2
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

    ALIGN_STACK_AND_SAVE

    mov rdi, %1
    call %2

    call end_of_irq_ack_fn_wrapper

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
    sti
    iretq
%endmacro


%endif