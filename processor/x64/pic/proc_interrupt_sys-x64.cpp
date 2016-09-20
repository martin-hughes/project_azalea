/// @file
/// @brief Code for managing the PICs (of all types) attached to the system.
///
/// This code is as generic an interface as possible for the various types of Programmable Interrupt Controller that
/// may be attached to an x64 system. During initialisation, it selects the most advanced mode that it supports.

//#define ENABLE_TRACING

#include "processor/x64/pic/pic.h"
#include "processor/x64/pic/apic.h"
#include "processor/x64/pic/ioapic-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "klib/klib.h"

// The various PIC types supported by the Project Azalea kernel.
enum class APIC_TYPES { LEGACY_PIC, APIC, X2APIC };

const unsigned long APIC_PRESENT = 0x0000020000000000;
const unsigned long X2_APIC_PRESENT = 0x0000000000200000;

APIC_TYPES selected_pic_mode = APIC_TYPES::LEGACY_PIC;

APIC_TYPES proc_x64_detect_pic_type();

/// @brief Select an interrupt control system for the system to use
///
/// The choices currently are to use the Legacy PIC, or APIC. X2APIC systems use the normal APIC.
///
/// @param num_procs The number of processors attached to the system.
void proc_conf_interrupt_control_sys(unsigned int num_procs)
{
  KL_TRC_ENTRY;

  APIC_TYPES local_pic = proc_x64_detect_pic_type();

  switch (local_pic)
  {
    case APIC_TYPES::LEGACY_PIC:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Using legacy PIC mode\n");
      selected_pic_mode = APIC_TYPES::LEGACY_PIC;
      ASSERT(num_procs == 1);
      break;

    case APIC_TYPES::APIC:
    case APIC_TYPES::X2APIC:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Attempting to use APIC mode\n");
      selected_pic_mode = APIC_TYPES::APIC;
      proc_x64_configure_sys_apic_mode(num_procs);
      break;

    default:
      panic("Unsupported PIC type detected!");
  }

  KL_TRC_EXIT;
}

/// @brief Configure the interrupt controller attached to this processor.
///
/// Each processor will start their own (A)PIC. The system's IO-APICs are initialised separately.
void proc_conf_local_int_controller()
{
  KL_TRC_ENTRY;

  APIC_TYPES local_pic = proc_x64_detect_pic_type();

  switch (local_pic)
  {
    case APIC_TYPES::LEGACY_PIC:
      ASSERT(proc_mp_this_proc_id() == 0);
      ASSERT(selected_pic_mode == APIC_TYPES::LEGACY_PIC);
      asm_proc_configure_irqs();
      break;

    case APIC_TYPES::APIC:
    case APIC_TYPES::X2APIC:
      ASSERT(selected_pic_mode == APIC_TYPES::APIC);
      proc_x64_configure_local_apic_mode();
      break;

    default:
      panic("Unsupported PIC type");
  }

  KL_TRC_EXIT;
}

/// @brief Configure any interrupt controllers that are not local to a specific processor.
///
/// For the time being, that's only expected to be the IO-APICs.
void proc_configure_global_int_ctrlrs()
{
  KL_TRC_ENTRY;

  unsigned char bsp_apic_id = proc_x64_apic_get_local_id();

  proc_x64_ioapic_load_data();

  if (selected_pic_mode != APIC_TYPES::LEGACY_PIC)
  {
    // If there's no legacy PIC, then there must be both an APIC and IO-APIC. The IO-APIC, being a system-wide (global)
    // interrupt controller, still needs its interrupts remapping. If we're in legacy PIC mode, this has been done
    // already (since the PIC is attached to the processor)
    ASSERT(proc_x64_ioapic_get_count() > 0);

    // Remap what would have been called IRQ 0-15 into interrupts 32-47. Point them all towards the BSP for now.
    proc_x64_ioapic_remap_interrupts(0, 32, bsp_apic_id);
  }

  // Some more stuff too, but I don't know what yet.

  KL_TRC_EXIT;
}

/// @brief Detect which type of PIC is attached to this processor.
///
/// For the time being, it is assumed that all processors have the same type of PIC as this one.
APIC_TYPES proc_x64_detect_pic_type()
{
  KL_TRC_ENTRY;

  APIC_TYPES detected_pic = APIC_TYPES::LEGACY_PIC;
  unsigned long apic_det_result;
  unsigned long ebx_eax;
  unsigned long edx_ecx;

  asm_proc_read_cpuid(1, 0, &ebx_eax, &edx_ecx);
  KL_TRC_DATA("CPUID EBX:EAX", ebx_eax);
  KL_TRC_DATA("CPUID EDX:ECX", edx_ecx);

  if ((edx_ecx & APIC_PRESENT) != 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "APIC Present. Looking for X2 APIC\n");
    if ((edx_ecx & X2_APIC_PRESENT) != 0)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "x2APIC present\n");
      detected_pic = APIC_TYPES::X2APIC;
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Regular APIC/xAPIC\n");
      detected_pic = APIC_TYPES::APIC;
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "No APIC detected - using legacy PIC\n");
    detected_pic = APIC_TYPES::LEGACY_PIC;
  }

  KL_TRC_EXIT;

  return detected_pic;
}

/// @brief Send an IPI to another processor
///
/// A more detailed description of the meaning of these parameters can be found in the Intel System Programming Guide.
///
/// @param apic_dest The ID of the APIC to send the IPI to. May be zero if a shorthand is used.
///
/// @param shorthand If needed, the shorthand code for signalling multiple processors at once
///
/// @param interrupt The desired type of IPI to send
///
/// @param vector The vector number for this IPI. Depending on the type of IPI being sent, this may be ignored.
void proc_send_ipi(unsigned int apic_dest,
                   PROC_IPI_SHORT_TARGET shorthand,
                   PROC_IPI_INTERRUPT interrupt,
                   unsigned char vector)
{
  KL_TRC_ENTRY;

  ASSERT(selected_pic_mode == APIC_TYPES::APIC || selected_pic_mode == APIC_TYPES::X2APIC);

  switch (selected_pic_mode)
  {
    case APIC_TYPES::APIC:
      proc_apic_send_ipi(apic_dest, shorthand, interrupt, vector);
      break;

    case APIC_TYPES::X2APIC:
      panic("X2 APIC mode not yet supported");
      break;
  }

  KL_TRC_EXIT;
}
