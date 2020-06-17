[BITS 64]
SEGMENT .text

; This file contains functions for enabling and configuring the PICs.
; The code for acknowledging IRQs is in interrups-low-x64.asm.
;
; Most of this code is only used if the system has no APIC to use instead.
;
; TODO: Make the IRQ->Interrupt mapping not depend on magic numbers.

%macro OUTB 2
    mov al, %2
    out %1, al
%endmacro

GLOBAL asm_proc_configure_irqs
extern end_of_irq_ack_fn
; Start by configuring the PICs.
asm_proc_configure_irqs:
    push rax
    push rcx
    push rdx
    ; PIC initialisation. Configures the IRQs to be from int 32 to 47, inclusive.
    OUTB 0x20, 0x11
    OUTB 0xA0, 0x11
    OUTB 0x21, 0x20
    OUTB 0xA1, 0x28
    OUTB 0x21, 0x04
    OUTB 0xA1, 0x02
    OUTB 0x21, 0x01
    OUTB 0xA1, 0x01
    OUTB 0x21, 0x0
    OUTB 0xA1, 0x0

    ; Disable the Local APIC if one exists. This might seem unnecesary, but it covers the case where a system has a
    ; Local APIC but no IO APIC - in which case, the kernel falls back to Legacy PIC mode.
    call asm_proc_disable_local_apic

    mov rax, asm_proc_legacy_pic_irq_ack
    mov rbx, end_of_irq_ack_fn
    mov [rbx], rax

    pop rdx
    pop rcx
    pop rax
    ret

asm_proc_disable_local_apic:
    mov rax, 1
    cpuid
    mov rcx, 0x200 ; TODO: magic flag!

    and rdx, rcx
    cmp rdx, 0
    je no_apic

    ; There's an APIC, so disable it.
    mov rcx, 0x1B ; TODO: Magic constant
    rdmsr

    mov r8, 0x0800 ; TODO: And another!
    not r8
    and rax, r8

    wrmsr

    no_apic:
    ret

; Disables the legacy PIC by masking all interrupts.
; TODO: Is it necessary to still remap IRQs in order to prevent spurious exceptions being triggered?
GLOBAL asm_proc_disable_legacy_pic
asm_proc_disable_legacy_pic:
  mov al, 0xff
  out 0xa1, al
  out 0x21, al

  ret

GLOBAL asm_proc_legacy_pic_irq_ack
asm_proc_legacy_pic_irq_ack:
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret
