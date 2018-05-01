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

    ; Save the execution pointer as a parameter for task_int_swap_task.
    mov rsi, cr3

    ; Move the stack pointer down a bunch. The processor aligns the stack to 16-bytes before beginning the interrupt
    ; instruction, then adds five pushes. We've added a further 15, so the stack is still 16-byte aligned.
    sub rsp, 512
    fxsave64 [rsp]

    ; Save the stack pointer as a parameter for task_int_swap_task.
    mov rdi, rsp

    ; Execute the task swap.
    call task_int_swap_task

    ; From the returned execution context structure, compute the correct value of CR3 and restore it. CR3 is the first
    ; element of the structure.
    mov rax, [rax]
    mov cr3, rax

    ; Restore the FX state and put the stack back to the correct place.
    fxrstor64 [rsp]
    add rsp, 512

    ; Task switching complete, release the lock.
    mov rax, task_switch_lock
    mov rbx, 0
    mov [rax], rbx

    ; Need to do an indirect call, which works from RAX, so store it, call and restore.
    push rax
    mov rax, end_of_irq_ack_fn
    mov rax, [rax]
    call rax
    pop rax

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
    iretq
