[BITS 64]
SEGMENT .text

task_switch_lock:
    dq 0x00

EXTERN task_int_swap_task
EXTERN asm_klib_synch_spinlock_lock
GLOBAL asm_task_switch_interrupt
extern end_of_irq_ack_fn

; Note that throughout this code, the manipulations of the stack must match those in task_int_create_exec_context, or
; the process will crash as soon as it is started!
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

    mov rdi, task_switch_lock
    call asm_klib_synch_spinlock_lock

    ; Save the stack pointer and prepare the execution pointer and CR3 as parameters for task_int_swap_task.
    mov rdi, rsp
    mov rsi, [rsp + 128]
    mov rdx, cr3

    ; Move the stack pointer down a bunch
    sub rsp, 512

    ; Align it to a 16-byte boundary
    and rsp, qword 0xFFFFFFFFFFFFFFF0
    fxsave64 [rsp]
    sub rsp, 8

    ; Save the original stack pointer, and update the one being passed as a parameter.
    push rdi
    mov rdi, rsp

    ; Execute the task swap.
    call task_int_swap_task

    ; Need to do an indirect call, which works from RAX, so store it, call and restore.
    push rax
    mov rax, end_of_irq_ack_fn
    call [rax]
    pop rax

    ; From the returned execution context structure, compute the correct value of CR3 and restore it. CR3 is the third
    ; entry.
    mov rdx, rax
    add rdx, 16
    mov rax, [rdx]
    mov cr3, rax

    ; From the same structure, restore the stack to its post-fxsave state (RSP is the first entry).
    sub rdx, 16
    mov rsp, [rdx]

    ; What was our original stack pointer?
    pop rdi

    ; Restore the FX state.
    add rsp, 8
    fxrstor [rsp]

    ; Restore the original stack.
    mov rsp, rdi

    ; Task switching complete, release the lock.
    mov rax, task_switch_lock
    mov rbx, 0
    mov [rax], rbx

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
