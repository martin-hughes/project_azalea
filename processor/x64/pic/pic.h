#ifndef __PROC_X64_PIC_AND_APIC
#define __PROC_X64_PIC_AND_APIC

// Interface for controlling the system's various PICs or APIC/xAPIC/x2APICs
void proc_conf_local_int_controller();
void proc_configure_global_int_ctrlrs();

extern "C" void asm_proc_configure_irqs();
extern "C" void asm_proc_disable_legacy_pic();
extern "C" void asm_proc_legacy_pic_irq_ack();

#endif
