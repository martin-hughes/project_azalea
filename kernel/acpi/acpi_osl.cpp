// ACPI OS Services Layer for Project Azalea.

//#define ENABLE_TRACING

#include <stdint.h>

#include "klib/klib.h"

extern "C"
{
#ifndef DOXYGEN_BUILD
#include "acpi.h"
#endif
}
#include "processor/processor.h"
#include "processor/timing/timing.h"
#include "processor/x64/processor-x64-int.h"
#include "mem/mem.h"
#include "devices/device_interface.h"
#include "devices/pci/pci_functions.h"

#include <stdio.h>

/// @brief Handle an IRQ as requested by ACPI.
///
/// An object of this class can then be registered with the IRQ handling system. It simply passes on the IRQ to
/// ACPICA to handle.
class AcpiIrqHandler : public IInterruptReceiver
{
public:
  AcpiIrqHandler(ACPI_OSD_HANDLER irq_handler, void *irq_context);
  virtual ~AcpiIrqHandler() { };

  virtual bool handle_interrupt_fast(uint8_t irq_number);
  virtual void handle_interrupt_slow(uint8_t irq_number) { };

private:
  ACPI_OSD_HANDLER _irq_handler;
  void *_irq_context;
};

namespace
{
  char *exception_message_buf = nullptr; ///< Somewhere to store messages output by ACPICA for printing.
  const uint16_t em_buf_len = 1000; ///< The number of bytes in the above buffer.

  // At present, only support a single IRQ handler for ACPI.
  AcpiIrqHandler *acpi_int_handler = nullptr; ///< ACPI's IRQ handler object.
}

// Don't bother re-documenting ACPICA.
/// @cond

// There are no actions needed to initialize the OSL, so just return.
ACPI_STATUS AcpiOsInitialize(void)
{
  KL_TRC_ENTRY;

  exception_message_buf = new char[em_buf_len];

  KL_TRC_EXIT;
  return AE_OK ;
}

ACPI_STATUS AcpiOsTerminate(void)
{
  KL_TRC_ENTRY;
  panic("Hit AcpiOsTerminate - should never be called.");
  KL_TRC_EXIT;

  return AE_OK;
}

/*
 * ACPI Table interfaces
 */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
  KL_TRC_ENTRY;
  ACPI_PHYSICAL_ADDRESS  Ret;
  Ret = 0;
  AcpiFindRootPointer(&Ret);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Root pointer: ", Ret, "\n");
  KL_TRC_EXIT;
  return Ret;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal)
{
  KL_TRC_ENTRY;
  *NewVal = (ACPI_STRING)nullptr;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
  KL_TRC_ENTRY;
  *NewTable = (ACPI_TABLE_HEADER *)nullptr;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength)
{
  KL_TRC_ENTRY;
  *NewAddress = (ACPI_PHYSICAL_ADDRESS)nullptr;
  KL_TRC_EXIT;

  return AE_OK;
}

/*
 * Spinlock primitives
 */
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = new kernel_spinlock;
  klib_synch_spinlock_init(*lock);
  *OutHandle = (ACPI_SPINLOCK)lock;
  KL_TRC_EXIT;

  return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != nullptr);
  delete lock;
  KL_TRC_EXIT;
}

// The ACPI_CPU_FLAGS return value is simply passed to the AcpiOsReleaseLock function, so can be ignored.
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != nullptr);
  klib_synch_spinlock_lock(*lock);
  KL_TRC_EXIT;

  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != nullptr);
  klib_synch_spinlock_unlock(*lock);
  KL_TRC_EXIT;
}

/*
 * Semaphore primitives
 */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
  KL_TRC_ENTRY;
  klib_semaphore *sp = new klib_semaphore;
  klib_synch_semaphore_init(*sp, MaxUnits, InitialUnits);
  *OutHandle = (ACPI_SEMAPHORE)sp;

  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
  KL_TRC_ENTRY;
  klib_semaphore *sp = (klib_semaphore *)Handle;
  ASSERT(sp != nullptr);
  delete sp;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
  KL_TRC_ENTRY;
  uint64_t wait = Timeout;
  SYNC_ACQ_RESULT res;
  ACPI_STATUS retval = AE_ERROR;
  klib_semaphore *sp = (klib_semaphore *)Handle;

  if (sp == nullptr) return AE_BAD_PARAMETER;
  ASSERT(sp != nullptr);
  ASSERT(Units == 1);

  if (wait == 0xFFFF)
  {
    wait = MUTEX_MAX_WAIT;
  }

  res = klib_synch_semaphore_wait(*sp, wait);

  switch (res)
  {
    case SYNC_ACQ_ACQUIRED:
      retval = AE_OK;
      break;

    case SYNC_ACQ_TIMEOUT:
      retval = AE_TIME;
      break;

    default:
      panic("Unknown semaphore result");

  }
  KL_TRC_EXIT;

  return retval;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
  KL_TRC_ENTRY;
  klib_semaphore *sp = (klib_semaphore *)Handle;
  ASSERT(sp != nullptr);
  ASSERT(Units == 1);

  klib_synch_semaphore_clear(*sp);
  KL_TRC_EXIT;

  return AE_OK;
}

/*
 * Mutex primitives. May be configured to use semaphores instead via
 * ACPI_MUTEX_TYPE (see platform/acenv.h)
 */
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = new klib_mutex;
  klib_synch_mutex_init(*mutex);
  *OutHandle = (ACPI_MUTEX)mutex;
  KL_TRC_EXIT;

  return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != nullptr);
  delete mutex;
  KL_TRC_EXIT;
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = (klib_mutex *)Handle;
  uint64_t wait = Timeout;
  ACPI_STATUS retval = AE_ERROR;
  SYNC_ACQ_RESULT res;
  ASSERT(mutex != nullptr);

  if (wait == 0xFFFF)
  {
    wait = MUTEX_MAX_WAIT;
  }

  res = klib_synch_mutex_acquire(*mutex, wait);

  switch (res)
  {
    case SYNC_ACQ_ACQUIRED:
      KL_TRC_TRACE(TRC_LVL::FLOW, "Acq\n");
      retval = AE_OK;
      break;

    case SYNC_ACQ_TIMEOUT:
      retval = AE_TIME;
      KL_TRC_TRACE(TRC_LVL::FLOW, "Timeout!");
      break;

    default:
      panic("Unknown mutex result");

  }
  KL_TRC_EXIT;

  return retval;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != nullptr);

  klib_synch_mutex_release(*mutex, false);
  KL_TRC_EXIT;
}

/*
 * Memory allocation and mapping
 */
void *AcpiOsAllocate(ACPI_SIZE Size)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
  return (void *)(new char[Size]);
}

void AcpiOsFree(void * Memory)
{
  KL_TRC_ENTRY;
  ASSERT(Memory != nullptr);
  delete[] (char *)Memory;
  KL_TRC_EXIT;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length)
{
  KL_TRC_ENTRY;
  uint64_t start_of_page;
  uint64_t offset;
  uint64_t total_length;
  uint32_t num_pages;
  uint64_t start_of_range;

  offset = (uint64_t)Where % MEM_PAGE_SIZE;
  start_of_page = (uint64_t)Where - offset;
  total_length = (uint64_t)Length + offset;
  num_pages = (total_length / MEM_PAGE_SIZE) + 1;

  start_of_range = (uint64_t)mem_allocate_virtual_range(num_pages);
  mem_map_range((void *)start_of_page, (void *)start_of_range, num_pages);
  start_of_range += offset;
  KL_TRC_EXIT;

  return (void *)start_of_range;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size)
{
  KL_TRC_ENTRY;
  uint64_t addr;
  uint64_t start_of_page;
  uint64_t offset;
  uint64_t total_length;
  uint64_t num_pages;

  addr = (uint64_t)LogicalAddress;
  offset = addr % MEM_PAGE_SIZE;
  start_of_page = addr - offset;
  total_length = Size + offset;
  num_pages = (total_length / MEM_PAGE_SIZE) + 1;

  mem_unmap_range((void *)start_of_page, num_pages, nullptr, false);
  mem_deallocate_virtual_range((void *)start_of_page, num_pages);
  KL_TRC_EXIT;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
  KL_TRC_ENTRY;

  ACPI_STATUS ret = AE_OK;
  void *translated;

  if ((LogicalAddress == nullptr) || (PhysicalAddress == nullptr))
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Invalid parameters given\n");
    ret = AE_BAD_PARAMETER;
  }
  else
  {
    translated = mem_get_phys_addr(LogicalAddress);
    *PhysicalAddress = reinterpret_cast<ACPI_PHYSICAL_ADDRESS>(translated);

    KL_TRC_TRACE(TRC_LVL::FLOW, "Translated ", LogicalAddress, " into ", translated, "\n");
  }

  KL_TRC_EXIT;

  return ret;
}

/*
 * Interrupt handlers
 */
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
  // Dispute the naming in the ACPICA headers, it looks as though InterruptNumber is actually IRQ number.
  KL_TRC_ENTRY;

  ACPI_STATUS ret = AE_OK;

  ASSERT(acpi_int_handler == nullptr);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Interrupt number: ", InterruptNumber, "\n");

  if (InterruptNumber > 15)
  {
    ASSERT(false);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Tried to use an interrupt number that we've reserved\n");
    ret = AE_BAD_PARAMETER;
  }
  else
  {
    acpi_int_handler = new AcpiIrqHandler(ServiceRoutine, Context);
    proc_register_irq_handler(InterruptNumber, dynamic_cast<IInterruptReceiver *>(acpi_int_handler));
  }

  KL_TRC_EXIT;
  return ret;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine)
{
  KL_TRC_ENTRY;
  ACPI_STATUS ret = AE_OK;

  ASSERT(acpi_int_handler != nullptr);
  KL_TRC_TRACE(TRC_LVL::FLOW, "Interrupt number: ", InterruptNumber, "\n");

  if (InterruptNumber > 15)
  {
    ASSERT(false);
    KL_TRC_TRACE(TRC_LVL::FLOW, "Tried to use an interrupt number that we've reserved\n");
    ret = AE_BAD_PARAMETER;
  }
  else
  {
    proc_unregister_irq_handler(InterruptNumber, acpi_int_handler);
    delete acpi_int_handler;
    acpi_int_handler = nullptr;
  }

  KL_TRC_EXIT;
  return AE_OK;
}

/*
 * Threads and Scheduling
 */
ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
  uint64_t thread_id;
  KL_TRC_ENTRY;
  thread_id = (uint64_t)task_get_cur_thread();

  // If task_get_cur_thread returns 0 then we're still single threaded, in which case it's acceptable to send any
  // positive integer back to ACPICA.
  if (thread_id == 0)
  {
    thread_id = 1;
  }
  KL_TRC_EXIT;

  return thread_id;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(AcpiOsExecute);
  KL_TRC_EXIT;
  return AE_OK;
}

// Don't know quite what this does...
void AcpiOsWaitEventsComplete(void)
{
  KL_TRC_ENTRY;
  panic("AcpiOsWaitEventsComplete - wtf??");
  KL_TRC_EXIT;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
  KL_TRC_ENTRY;

  uint64_t wait_in_ns = Milliseconds * 1000000;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "ACPI requests sleep (ns)", wait_in_ns, "\n");
  time_sleep_process(wait_in_ns);
  KL_TRC_EXIT;
}

void AcpiOsStall(UINT32 Microseconds)
{
  KL_TRC_ENTRY;
  uint64_t wait_in_ns = Microseconds * 1000;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "ACPI requests stall (ns)", wait_in_ns, "\n");
  time_stall_process(wait_in_ns);

  KL_TRC_EXIT;
}

/*
 * Platform and hardware-independent I/O interfaces
 */
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Address", Address, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Output address", Value, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Width", Width, "\n");

  ASSERT(Value != (UINT32 *)nullptr);
  ASSERT((Width == 8) || (Width == 16) || (Width == 32));

  *Value = proc_read_port(Address, Width);

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Value returned", *Value, "\n");

  KL_TRC_EXIT;
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
  KL_TRC_ENTRY;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Address", Address, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Value", Value, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Width", Width, "\n");

  ASSERT((Width == 8) || (Width == 16) || (Width == 32));

  proc_write_port(Address, Value, Width);

  KL_TRC_EXIT;
  return AE_OK;
}

/*
 * Platform and hardware-independent physical memory interfaces
 */
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
  KL_TRC_ENTRY;
  uint64_t *mem = (uint64_t *)AcpiOsMapMemory(Address, Width / 8);
  *Value = *mem;
  AcpiOsUnmapMemory((void *)mem, Width / 8);
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
  KL_TRC_ENTRY;
  uint64_t *mem = (uint64_t *)AcpiOsMapMemory(Address, Width / 8);

  kl_memcpy(&Value, (void *)mem, Width / 8);

  AcpiOsUnmapMemory((void *)mem, Width / 8);
  KL_TRC_EXIT;

  return AE_OK;
}

/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 *Value, UINT32 Width)
{
  uint32_t raw_reg;
  uint32_t raw_reg_upper;

  KL_TRC_ENTRY;

  // PCI Express isn't currently supported.
  ASSERT(PciId->Segment == 0);

  pci_address addr;
  addr.bus = PciId->Bus;
  addr.device = PciId->Device;
  addr.function = PciId->Function;
  addr.register_num = Reg;

  raw_reg = pci_read_raw_reg(addr);

  switch(Width)
  {
  case 8:
    KL_TRC_TRACE(TRC_LVL::FLOW, "8-bit width\n");
    *Value = raw_reg & 0xFF;
    break;

  case 16:
    KL_TRC_TRACE(TRC_LVL::FLOW, "16-bit width\n");
    *Value = raw_reg & 0xFFFF;
    break;

  case 32:
    KL_TRC_TRACE(TRC_LVL::FLOW, "32-bit width\n");
    *Value = raw_reg;
    break;

  case 64:
    KL_TRC_TRACE(TRC_LVL::FLOW, "64-bit width\n");
    addr.register_num++;
    raw_reg_upper = pci_read_raw_reg(addr);
    *Value = raw_reg_upper;
    *Value = *Value << 32;
    *Value = *Value | raw_reg;
    break;

  default:
    panic("Unknown PCI register width");
  }

  KL_TRC_EXIT;
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
  uint32_t raw_reg;

  KL_TRC_ENTRY;

  // PCI Express isn't currently supported.
  ASSERT(PciId->Segment == 0);

  pci_address addr;
  addr.bus = PciId->Bus;
  addr.device = PciId->Device;
  addr.function = PciId->Function;
  addr.register_num = Reg;

  raw_reg = pci_read_raw_reg(addr);

  switch(Width)
  {
  case 8:
    KL_TRC_TRACE(TRC_LVL::FLOW, "8-bit width\n");
    raw_reg = raw_reg & 0xFFFFFF00;
    raw_reg = raw_reg | (Value & 0xFF);
    pci_write_raw_reg(addr, raw_reg);
    break;

  case 16:
    KL_TRC_TRACE(TRC_LVL::FLOW, "16-bit width\n");
    raw_reg = raw_reg & 0xFFFF0000;
    raw_reg = raw_reg | (Value & 0xFFFF);
    pci_write_raw_reg(addr, raw_reg);
    break;

  case 32:
    KL_TRC_TRACE(TRC_LVL::FLOW, "32-bit width\n");
    raw_reg = Value;
    pci_write_raw_reg(addr, raw_reg);
    break;

  case 64:
    KL_TRC_TRACE(TRC_LVL::FLOW, "64-bit width\n");
    raw_reg = Value;
    pci_write_raw_reg(addr, raw_reg);
    Value = Value >> 32;
    raw_reg = Value;
    addr.register_num++;
    pci_write_raw_reg(addr, raw_reg);
    break;

  default:
    panic("Unknown PCI register width");
  }

  KL_TRC_EXIT;
  return AE_OK;
}

/*
 * Miscellaneous
 */
BOOLEAN AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
  KL_TRC_ENTRY;

  uint64_t vp = reinterpret_cast<uint64_t>(Pointer);

  bool res = (mem_get_phys_addr(reinterpret_cast<void *>(vp)) != nullptr) &&
             (mem_get_phys_addr(reinterpret_cast<void *>(vp + Length)) != nullptr);

  KL_TRC_EXIT;
  return res ? TRUE : FALSE;
}

BOOLEAN AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
  // For the time being, memory that is readable is writable.
  return AcpiOsReadable(Pointer, Length);
}

UINT64 AcpiOsGetTimer(void)
{
  UINT64 timer_val;
  KL_TRC_ENTRY;

  timer_val = time_get_system_timer_count(true) / 100;
  KL_TRC_EXIT;

  return timer_val;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to signal function");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

/*
 * Debug print routines
 */
void AcpiOsPrintf(const char *Format, ...)
{
  //KL_TRC_ENTRY;
  va_list args;

  va_start(args, Format);
  AcpiOsVprintf(Format, args);
  va_end(args);

  //KL_TRC_EXIT;
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
  //KL_TRC_ENTRY;
  if (!exception_message_buf)
  {
    return;
  }

  kl_memset(exception_message_buf, 0, 1000);
  vsnprintf(exception_message_buf, em_buf_len, Format, Args);

  kl_trc_trace(TRC_LVL::EXTRA, (const char *)exception_message_buf);
  //KL_TRC_EXIT;
}

void AcpiOsRedirectOutput(void *Destination)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted output change");
  KL_TRC_EXIT;
}

/*
 * Debug input
 */
ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to read keyboard");
  KL_TRC_EXIT;
  return AE_OK;
}

/*
 * Obtain ACPI table(s)
 */
ACPI_STATUS AcpiOsGetTableByName(char *Signature, UINT32 Instance, ACPI_TABLE_HEADER **Table, ACPI_PHYSICAL_ADDRESS *Address)
{
  KL_TRC_ENTRY;
  panic("Attempting to fetch table by name");
  KL_TRC_EXIT;
  return AE_NO_ACPI_TABLES;
}

ACPI_STATUS AcpiOsGetTableByIndex(UINT32 Index, ACPI_TABLE_HEADER **Table, UINT32 *Instance, ACPI_PHYSICAL_ADDRESS *Address)
{
  KL_TRC_ENTRY;
  panic("Attempting to fetch table by index");
  KL_TRC_EXIT;
  return AE_NO_ACPI_TABLES;
}

ACPI_STATUS AcpiOsGetTableByAddress(ACPI_PHYSICAL_ADDRESS Address, ACPI_TABLE_HEADER **Table)
{
  KL_TRC_ENTRY;
  panic("Attempting to fetch table by address");
  KL_TRC_EXIT;
  return AE_NO_ACPI_TABLES;
}

/*
 * Directory manipulation
 */
void *AcpiOsOpenDirectory(char *Pathname, char *WildcardSpec, char RequestedFileType)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to open directory");
  KL_TRC_EXIT;
  return nullptr;
}

char *AcpiOsGetNextFilename(void *DirHandle)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to enumerate directory");
  KL_TRC_EXIT;
  return (char *)nullptr;
}

void AcpiOsCloseDirectory(void *DirHandle)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to close directory");
  KL_TRC_EXIT;
}

/*
 * File I/O and related support
 */
ACPI_FILE AcpiOsOpenFile (const char *Path, UINT8 Modes)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to open file");
  KL_TRC_EXIT;
  return nullptr;
}

void AcpiOsCloseFile(ACPI_FILE File)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to close file");
  KL_TRC_EXIT;
}

int AcpiOsReadFile(ACPI_FILE File, void *Buffer, ACPI_SIZE Size, ACPI_SIZE Count)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to read file");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

int AcpiOsWriteFile(ACPI_FILE File, void *Buffer, ACPI_SIZE Size, ACPI_SIZE Count)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to write file");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

long AcpiOsGetFileOffset(ACPI_FILE File)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to to find file offset");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsSetFileOffset(ACPI_FILE File, long Offset, UINT8 From)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to set file offset");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

void AcpiOsTracePoint(ACPI_TRACE_EVENT_TYPE Type, BOOLEAN Begin, UINT8 *Aml, char *Pathname)
{
  KL_TRC_ENTRY;
  panic("ACPI trace point called");
  KL_TRC_EXIT;
}

/// @endcond

AcpiIrqHandler::AcpiIrqHandler(ACPI_OSD_HANDLER irq_handler, void *irq_context) :
  _irq_handler(irq_handler),
  _irq_context(irq_context)
{
}

bool AcpiIrqHandler::handle_interrupt_fast(uint8_t irq_number)
{
  // If ACPI IRQs start to give grief then note that this function returns a UINT and maybe it had some purpose after
  // all...
  _irq_handler(_irq_context);

  return false;
}
