/// @file
/// @brief Supports multi-processor operations.
///
/// Supports multi-processor operations.
/// Allows:
/// - Processors to be enumerated and identified
/// - Processors to be started and stopped
/// - Signals to be sent between processors.
///
/// Functions in this file that do not contain _x64 in their name would be generic to all platforms, but the exact
/// implementation is platform specific.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "processor/processor.h"
#include "processor/processor-int.h"
#include "processor/x64/processor-x64.h"
#include "processor/x64/processor-x64-int.h"
#include "processor/x64/pic/pic.h"
#include "processor/x64/pic/apic.h"
#include "acpi/acpi_if.h"
#include "mem/x64/mem-x64.h"
#include "syscall/x64/syscall_kernel-x64.h"
#include "processor/timing/timing.h"

/// @brief Controls communication between source and target processors.
enum class PROC_MP_X64_MSG_STATE
{
  /// The default is this state. If a target processor receives an NMI and this is the state then it wasn't generated
  /// by the kernel to signal messages. Once the source processor receives its acknowledgement it should set this
  /// value again.
  NO_MSG,

  /// Tells the target processor that a message is waiting for it.
  MSG_WAITING,

  /// The target processor sets this value after dealing with its IPI in order to let the source know it has done its
  /// work.
  ACKNOWLEDGED,
};

/// A structure for storing details of inter-processor communications
struct proc_mp_ipi_msg_state
{
  /// The message sent by the initiator of communication.
  PROC_IPI_MSGS msg_being_sent;

  /// The current state of the communication. See the documentation of PROC_MP_X64_MSG_STATE for more details.
  volatile PROC_MP_X64_MSG_STATE msg_control_state;

  /// Prevents more than one processor signalling the target at once. Controlled by the initiator
  kernel_spinlock signal_lock;
};

const unsigned char SUBTABLE_LAPIC_TYPE = 0;
const unsigned short *pure_64_nmi_idt_entry = reinterpret_cast<const unsigned short *>(0xFFFFFFFF00000020);

static proc_mp_ipi_msg_state *inter_proc_signals = nullptr;

extern "C" unsigned long asm_proc_pure64_nmi_trampoline_start;
extern "C" unsigned long asm_proc_pure64_nmi_trampoline_end;

static_assert(sizeof(unsigned short) == 2, "Sizeof short must be two");

// Pointers to kernel stacks, one per processor. This allows each processor to enter syscall with its own stack.
void **kernel_syscall_stack_ptrs;

/// @brief Prepare the system to start multi-processing
///
/// Counts up the others processors and gathers useful information, but doesn't signal them to start just yet.
void proc_mp_init()
{
  KL_TRC_ENTRY;

  ACPI_STATUS retval;
  char table_name[] = "APIC";
  acpi_table_madt *madt_table;
  acpi_subtable_header *subtable;
  acpi_madt_local_apic *lapic_table;
  unsigned int procs_found = 0;
  unsigned long pure64_nmi_handler_loc;
  unsigned int trampoline_length;

  processor_count = *boot_info_cpu_cores_active;
  KL_TRC_DATA("Number of processors", processor_count);

  proc_info_block = new processor_info[processor_count];
  inter_proc_signals = new proc_mp_ipi_msg_state[processor_count];
  kernel_syscall_stack_ptrs = new void *[processor_count];

  retval = AcpiGetTable((ACPI_STRING)table_name, 0, (ACPI_TABLE_HEADER **)&madt_table);
  ASSERT(retval == AE_OK);
  ASSERT(madt_table->Header.Length > sizeof(acpi_table_madt));

  subtable = acpi_init_subtable_ptr((void *)madt_table, sizeof(acpi_table_madt));
  while(((unsigned long)subtable - (unsigned long)madt_table) < madt_table->Header.Length)
  {
    KL_TRC_DATA("Found a new table of type", (unsigned long)subtable->Type);

    if (subtable->Type == SUBTABLE_LAPIC_TYPE)
    {
      ASSERT(procs_found < processor_count);

      lapic_table = (acpi_madt_local_apic *)subtable;

      proc_info_block[procs_found].processor_id = procs_found;
      proc_info_block[procs_found].processor_running = false;
      proc_info_block[procs_found].platform_data.lapic_id = lapic_table->Id;

      KL_TRC_DATA("Our processor ID", procs_found);
      KL_TRC_DATA("ACPI proc ID", (unsigned long)lapic_table->ProcessorId);
      KL_TRC_DATA("LAPIC ID", (unsigned long)lapic_table->Id);

      procs_found++;
    }

    subtable = acpi_advance_subtable_ptr(subtable);
  }

  // Prepare the interrupt controllers for business.
  proc_conf_interrupt_control_sys(processor_count);
  proc_conf_local_int_controller();
  proc_configure_global_int_ctrlrs();

  ASSERT(procs_found == processor_count);

  // Fill in the inter-processor signal control codes. We have to fill in a valid signal, even though it isn't actually
  // being sent, so pick an arbitrary one. Processors should be protected from acting on it through the value of
  // msg_control_state.
  for (unsigned int i = 0; i < processor_count; i++)
  {
    KL_TRC_DATA("Filling in signals for proc", i);
    inter_proc_signals[i].msg_being_sent = PROC_IPI_MSGS::SUSPEND;
    inter_proc_signals[i].msg_control_state = PROC_MP_X64_MSG_STATE::NO_MSG;
    klib_synch_spinlock_init(inter_proc_signals[i].signal_lock);

    // Generate a stack for syscall to use. Remember that the stack grows downwards from the end of the allocation.
    kernel_syscall_stack_ptrs[i] = proc_x64_allocate_stack();

  }

  // The processors have been left halted with interrupts disabled by the bootloader. Short of a full reset of them the
  // only way to signal the APs is by NMI, but at the moment that handler calls Pure64 code. Cheat by redirecting it to
  // our handler. Start by finding exactly where the Pure64 NMI handler is.
  pure64_nmi_handler_loc = (unsigned long)pure_64_nmi_idt_entry[0] |
                           ((unsigned long)pure_64_nmi_idt_entry[3] << 16) |
                           ((unsigned long)pure_64_nmi_idt_entry[4] << 32) |
                           ((unsigned long)pure_64_nmi_idt_entry[5] << 48);
  KL_TRC_DATA("Pure64 NMI Handler location", pure64_nmi_handler_loc);

  // The address that has just been calculated assumes that physical and virtual addresses are equal, but we've loaded
  // in the higher half...
  pure64_nmi_handler_loc |= 0xFFFFFFFF00000000;

  // There's a short trampoline written in assembly language that is simply copied straight over the Pure64 NMI
  // handler.
  trampoline_length = reinterpret_cast<unsigned long>(&asm_proc_pure64_nmi_trampoline_end) -
                      reinterpret_cast<unsigned long>(&asm_proc_pure64_nmi_trampoline_start);
  KL_TRC_DATA("Trampoline start", reinterpret_cast<unsigned long>(&asm_proc_pure64_nmi_trampoline_start));
  KL_TRC_DATA("Trampoline length", trampoline_length);

  kl_memcpy(reinterpret_cast<void *>(&asm_proc_pure64_nmi_trampoline_start),
            reinterpret_cast<void *>(pure64_nmi_handler_loc),
            trampoline_length);

  // Recreate the GDT so that it is long enough to contain TSS descriptors for all processors
  proc_recreate_gdt(processor_count);

  // The first processor is definitely running already!
  proc_info_block[0].processor_running = true;

  // The APs have had their NMI handlers overwritten, ready to go. They are triggered in to life by proc_mp_start_aps()
  // Now all interrupt controllers needed for the BSP are good to go. Enable interrupts.
  asm_proc_start_interrupts();

  KL_TRC_EXIT;
}

/// @brief Application Processor (AP) startup code.
///
/// When this function is complete, the AP it is running on will be able to participate fully in the scheduling system.
void proc_mp_ap_startup()
{
  KL_TRC_ENTRY;

  unsigned int proc_num = proc_mp_this_proc_id();

  ASSERT(proc_num != 0);

  asm_proc_install_idt();
  mem_x64_pat_init();
  asm_syscall_x64_prepare();
  asm_proc_load_gdt();
  proc_load_tss(proc_mp_this_proc_id());
  proc_conf_local_int_controller();

  proc_info_block[proc_num].processor_running = true;

  // Signal completion to the signalling processor.
  if (inter_proc_signals[proc_num].msg_being_sent == PROC_IPI_MSGS::RESUME)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Expected startup message received\n");
    ASSERT(inter_proc_signals[proc_num].msg_control_state == PROC_MP_X64_MSG_STATE::MSG_WAITING);
    inter_proc_signals[proc_num].msg_control_state = PROC_MP_X64_MSG_STATE::ACKNOWLEDGED;
  }
  else
  {
    ASSERT(inter_proc_signals[proc_num].msg_control_state == PROC_MP_X64_MSG_STATE::NO_MSG);
  }

  // Starting interrupts ought to enable the processor to schedule work. If it doesn't start within a second, then
  // something has gone wrong.
  asm_proc_start_interrupts();

  KL_TRC_TRACE(TRC_LVL::FLOW, "Waiting for scheduling\n");
  time_stall_process(1000000000);
  panic("Failed to start AP");

  KL_TRC_EXIT;
}

/// @brief Return the ID number of this processor
///
/// Until multi-processing is supported, this will always return 0.
///
/// @return The integer ID number of the processor this function executes on.
unsigned int proc_mp_this_proc_id()
{
  bool apic_id_found = false;
  unsigned int lapic_id;
  unsigned int proc_id;

  KL_TRC_ENTRY;

  lapic_id = proc_x64_apic_get_local_id();

  KL_TRC_DATA("Looking for LAPIC ID", lapic_id);

  if (processor_count > 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Checking processor IDs\n");
    for (int i = 0; i < processor_count; i++)
    {
      if (lapic_id == proc_info_block[i].platform_data.lapic_id)
      {
        apic_id_found = true;
        proc_id = proc_info_block[i].processor_id;
        break;
      }
    }
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Not fully init'd, assume processor 0\n");
    apic_id_found = true;
    proc_id = 0;
  }

  ASSERT(apic_id_found);
  KL_TRC_DATA("Processor ID", proc_id);

  KL_TRC_EXIT;

  return proc_id;
}

/// @brief Send a IPI signal to another processor
///
/// Inter-processor interrupts are used to signal control messages between processors. Control messages are defined in
/// PROC_IPI_MSGS. x64 processors signal each other via NMI, which doesn't carry any information with it natively. So,
/// save information in a table so that the target can look it up again.
///
/// This function waits for the target processor to acknowledge the message before continuing.
///
/// @param proc_id The processor ID (not APIC ID) to signal.
///
/// @param msg The message to be sent.
void proc_mp_x64_signal_proc(unsigned int proc_id, PROC_IPI_MSGS msg)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Sending signal to processor", proc_id);
  KL_TRC_DATA("Message", static_cast<unsigned long>(msg));

  ASSERT(proc_id < processor_count);

  klib_synch_spinlock_lock(inter_proc_signals[proc_id].signal_lock);
  ASSERT(inter_proc_signals[proc_id].msg_control_state == PROC_MP_X64_MSG_STATE::NO_MSG);
  inter_proc_signals[proc_id].msg_being_sent = msg;
  inter_proc_signals[proc_id].msg_control_state = PROC_MP_X64_MSG_STATE::MSG_WAITING;

  proc_send_ipi(proc_info_block[proc_id].platform_data.lapic_id,
                PROC_IPI_SHORT_TARGET::NONE,
                PROC_IPI_INTERRUPT::NMI,
                0);

  while(inter_proc_signals[proc_id].msg_control_state != PROC_MP_X64_MSG_STATE::ACKNOWLEDGED)
  {
    // Spin while we wait.
  }

  inter_proc_signals[proc_id].msg_control_state = PROC_MP_X64_MSG_STATE::NO_MSG;
  klib_synch_spinlock_unlock(inter_proc_signals[proc_id].signal_lock);

  KL_TRC_EXIT;
}

/// @brief Receive and decode an IPI sent by another processor
///
/// In x64 land, inter processor signals are sent by signalling an NMI to the target. That carries no data with it, so
/// look up in the signal table to see what we received. Then pass that to the generic code to deal with it how it
/// likes
void proc_mp_x64_receive_signal_int()
{
  KL_TRC_ENTRY;

  unsigned int this_proc_id = proc_mp_this_proc_id();

  ASSERT(inter_proc_signals[this_proc_id].msg_control_state == PROC_MP_X64_MSG_STATE::MSG_WAITING);

  proc_mp_receive_signal(inter_proc_signals[this_proc_id].msg_being_sent);

  inter_proc_signals[this_proc_id].msg_control_state = PROC_MP_X64_MSG_STATE::ACKNOWLEDGED;

  KL_TRC_EXIT;
}
