//#define ENABLE_TRACING

#include "processor/x64/pic/pic.h"
#include "processor/x64/pic/apic.h"
#include "processor/x64/pic/ioapic-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "klib/klib.h"

// The various PIC types supported by 64bit.
enum APIC_TYPES { LEGACY_PIC, APIC, X2APIC };

const unsigned long APIC_PRESENT = 0x0000020000000000;
const unsigned long X2_APIC_PRESENT = 0x0000000000200000;

APIC_TYPES selected_pic_mode = LEGACY_PIC;

APIC_TYPES proc_x64_detect_pic_type();

// See what kind of interrupt controller (PIC, APIC, etc.) this system uses. Then configure it as appropriate.
// TODO: Consider multiple processors here. (MT)
// TODO: Enable x2APIC mode.
// TODO: Consider the possibility of multiple IOAPICs. (We use just one now) (STAB)?
// For now, just configure it on the local processor.


// Configure the interrupt controller attached to this processor. Each processor will start their own (A)PIC.
// The system's IO-APICs are initialized separately, by the main OS startup code.
void proc_conf_local_int_controller()
{
  KL_TRC_ENTRY;

  APIC_TYPES local_pic = proc_x64_detect_pic_type();

  switch (local_pic)
  {
    case LEGACY_PIC:
      KL_TRC_TRACE((TRC_LVL_FLOW, "Using legacy PIC mode\n"));
      selected_pic_mode = LEGACY_PIC;
      break;

    case APIC:
    case X2APIC:
      KL_TRC_TRACE((TRC_LVL_FLOW, "Attempting to use APIC mode\n"));
      selected_pic_mode = APIC;
      break;

    default:
      panic("Unsupported PIC type detected!");
  }

  switch (selected_pic_mode)
  {
    case LEGACY_PIC:
      asm_proc_configure_irqs();
      break;

    case APIC:
      proc_x64_configure_apic_mode();
      break;

    default:
      panic("Unsupported PIC type");
  }

  KL_TRC_EXIT;
}

// Configure any interrupt controllers that are not local to a specific processor. That's only expected to be the
// IO-APICs.
void proc_configure_global_int_ctrlrs()
{
  KL_TRC_ENTRY;

  unsigned char bsp_apic_id = proc_x64_apic_get_local_id();

  proc_x64_ioapic_load_data();

  if (selected_pic_mode != LEGACY_PIC)
  {
    // If there's no legacy PIC, then there must be both an APIC and IO-APIC. The IO-APIC, being a system-wide (global)
    // interrupt controller, still needs its interrupts remapping. If we're in legacy PIC mode, this has been done
    // already (since the PIC is attached to the processor)
    ASSERT(proc_x64_ioapic_get_count() > 0);

    // Remap what would have been called IRQ 0-15 into interrupts 32-47. Point them all towards the BSP for now.
    // TODO: The timer interrupt will definitely need to go to all processors (MT)
    proc_x64_ioapic_remap_interrupts(0, 32, bsp_apic_id);
  }

  // Some more stuff too, but I don't know what yet.

  KL_TRC_EXIT;
}

// Detect which type of PIC is attached to this processor.
// TODO: What happens if different processors have different types of PIC? (MT)
APIC_TYPES proc_x64_detect_pic_type()
{
  KL_TRC_ENTRY;

  APIC_TYPES detected_pic = LEGACY_PIC;
  unsigned long apic_det_result;
  unsigned long ebx_eax;
  unsigned long edx_ecx;

  asm_proc_read_cpuid(1, 0, &ebx_eax, &edx_ecx);
  KL_TRC_DATA("CPUID EBX:EAX", ebx_eax);
  KL_TRC_DATA("CPUID EDX:ECX", edx_ecx);

  if ((edx_ecx & APIC_PRESENT) != 0)
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "APIC Present. Looking for X2 APIC\n"));
    if ((edx_ecx & X2_APIC_PRESENT) != 0)
    {
      KL_TRC_TRACE((TRC_LVL_FLOW, "x2APIC present\n"));
      detected_pic = X2APIC;
    }
    else
    {
      KL_TRC_TRACE((TRC_LVL_FLOW, "Regular APIC/xAPIC\n"));
      detected_pic = APIC;
    }
  }
  else
  {
    KL_TRC_TRACE((TRC_LVL_FLOW, "No APIC detected - using legacy PIC\n"));
    detected_pic = LEGACY_PIC;
  }

  KL_TRC_EXIT;

  return detected_pic;
}
