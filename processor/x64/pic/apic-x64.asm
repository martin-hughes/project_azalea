[BITS 64]
SEGMENT .text

; Handler for spurious interrupts generated by the APIC.
GLOBAL asm_proc_apic_spurious_interrupt
asm_proc_apic_spurious_interrupt:
  iretq

