#ifndef __PROC_X64_PIC_AND_APIC
#define __PROC_X64_PIC_AND_APIC

// Interface for controlling the system's various PICs or APIC/xAPIC/x2APICs
void proc_conf_interrupt_control_sys(unsigned int num_procs);
void proc_conf_local_int_controller();
void proc_configure_global_int_ctrlrs();

extern "C" void asm_proc_configure_irqs();
extern "C" void asm_proc_disable_legacy_pic();
extern "C" void asm_proc_legacy_pic_irq_ack();

enum class PROC_IPI_SHORT_TARGET
{
  NONE = 0,
  SELF = 1,
  ALL_INCL_SELF = 2,
  ALL_EXCL_SELF = 3,
};

enum class PROC_IPI_INTERRUPT
{
  FIXED = 0,
  LOWEST_PRI = 1,
  SMI = 2,
  NMI = 4,
  INIT = 5,
  STARTUP = 6,
};

void proc_send_ipi(unsigned int apic_dest,
                   PROC_IPI_SHORT_TARGET shorthand,
                   PROC_IPI_INTERRUPT interrupt,
                   unsigned char vector,
                   const bool wait_for_delivery);

#endif
