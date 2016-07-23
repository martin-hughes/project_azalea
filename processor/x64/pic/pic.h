#ifndef __PROC_X64_PIC_AND_APIC
#define __PROC_X64_PIC_AND_APIC

// Interface for controlling the system's PIC or APIC/xAPIC/x2APIC

void proc_conf_int_controller();

extern "C" void asm_proc_configure_irqs();

#endif
