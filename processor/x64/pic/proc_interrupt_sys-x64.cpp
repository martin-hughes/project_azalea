#define ENABLE_TRACING

#include "processor/x64/pic/pic.h"
#include "processor/x64/processor-x64-int.h"
#include "klib/klib.h"

// The various PIC types supported by 64bit.
enum APIC_TYPES { LEGACY_PIC, APIC, X2APIC };

const unsigned long APIC_PRESENT = 0x0000020000000000;
const unsigned long X2_APIC_PRESENT = 0x0000000000200000;

APIC_TYPES selected_pic_mode = LEGACY_PIC;

APIC_TYPES proc_x64_detect_pic_type();
bool proc_x64_detect_io_apic();
void proc_x64_configure_apic_mode();
void proc_x64_configure_local_apic();
void proc_x64_io_apic_map_irq_range();
void proc_x64_io_apic_map_irq(unsigned short irq_num, unsigned short vector_num);
void proc_x64_disable_legacy_pic();

// See what kind of interrupt controller (PIC, APIC, etc.) this system uses. Then configure it as appropriate.
// TODO: Consider multiple processors here. (MT)
// TODO: Enable x2APIC mode.
// TODO: Consider the possibility of multiple IOAPICs. (We use just one now) (STAB)?
// For now, just configure it on the local processor.
void proc_conf_int_controller()
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
      if(proc_x64_detect_io_apic())
      {
        KL_TRC_TRACE((TRC_LVL_FLOW, "IO APIC detected, using APIC mode\n"));
        selected_pic_mode = APIC;
      }
      else
      {
        KL_TRC_TRACE((TRC_LVL_FLOW, "No IO APIC, fall back to legacy mode\n"));
        selected_pic_mode = LEGACY_PIC;
      }
      break;

    default:
      panic("Unsupported PIC type detected!");
  }

  // For the time being, always use the legacy PIC.
  // TODO: Remove this (MT)
  KL_TRC_TRACE((TRC_LVL_IMPORTANT, "FORCING LEGACY PIC MODE."));
  selected_pic_mode = LEGACY_PIC;

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

// Detects the presence of an IO APIC in the system.
bool proc_x64_detect_io_apic()
{
  INCOMPLETE_CODE(proc_x64_detect_io_apic);
  return false;
}

// Configures the system to use the APIC and IO APIC.
void proc_x64_configure_apic_mode()
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(proc_x64_configure_apic_mode);

  proc_x64_io_apic_map_irq_range();
  proc_x64_disable_legacy_pic();
  proc_x64_configure_local_apic();

  KL_TRC_EXIT;
}

// Configures the processor's local APIC.
void proc_x64_configure_local_apic()
{
  INCOMPLETE_CODE(proc_x64_configure_local_apic);
}

// Maps the IRQ range of 0-15 to interrupts 32-48.
// Note: If this range changes, it may be necessary to update the mapping in pic-x64.asm, and in interrupts-x64.cpp.
void proc_x64_io_apic_map_irq_range()
{
  KL_TRC_ENTRY;

  for (int i = 0; i < 16; i++)
  {
    proc_x64_io_apic_map_irq(i, i + 32);
  }

  KL_TRC_EXIT;
}

void proc_x64_io_apic_map_irq(unsigned short irq_num, unsigned short vector_num)
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE(proc_x64_io_apic_map_irq);

  KL_TRC_EXIT;
}

void proc_x64_disable_legacy_pic()
{
  KL_TRC_ENTRY;

  INCOMPLETE_CODE(proc_x64_diable_legacy_pic);

  KL_TRC_EXIT;
}
