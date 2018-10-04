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
