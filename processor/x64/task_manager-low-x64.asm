[BITS 64]
SEGMENT .text

EXTERN task_int_swap_task
GLOBAL asm_task_switch_interrupt
extern end_of_irq_ack_fn
asm_task_switch_interrupt:
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

    mov rdi, rsp
    mov rsi, 0
    mov rdx, cr3

    call task_int_swap_task

    mov rdx, rax
    add rdx, 16
    mov rax, [rdx]

    mov cr3, rax

	sub rdx, 16
    mov rsp, [rdx]

    mov rax, end_of_irq_ack_fn
    call [rax]

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
