//ACPI OS Services Layer for 64bit.
//
// At present this file does not contain KL_TRC_ENTRY / _EXIT calls, just in case it screws up ACPI. Maybe add them one
// day. Many of these functions are not supported, so they would just cause the kernel to panic. They shouldn't be
// needed!

extern "C"
{
#include "acpi/acpica/source/include/acpi.h"
}
#include "klib/klib.h"
#include "processor/timing/timing.h"

char *exception_message_buf;
const unsigned int em_buf_len = 1000;


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
  KL_TRC_EXIT;
  return Ret;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal)
{
  KL_TRC_ENTRY;
  *NewVal = (ACPI_STRING)NULL;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
  KL_TRC_ENTRY;
  *NewTable = (ACPI_TABLE_HEADER *)NULL;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength)
{
  KL_TRC_ENTRY;
  *NewAddress = (ACPI_PHYSICAL_ADDRESS)NULL;
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
  ASSERT(lock != NULL);
  delete lock;
  KL_TRC_EXIT;
}

// The ACPI_CPU_FLAGS return value is simply passed to the AcpiOsReleaseLock function, so can be ignored.
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != NULL);
  klib_synch_spinlock_lock(*lock);
  KL_TRC_EXIT;

  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
  KL_TRC_ENTRY;
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != NULL);
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
  ASSERT(sp != NULL);
  delete sp;
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
  KL_TRC_ENTRY;
  unsigned long wait = Timeout;
  SYNC_ACQ_RESULT res;
  ACPI_STATUS retval;
  klib_semaphore *sp = (klib_semaphore *)Handle;

  if (sp == NULL) return AE_BAD_PARAMETER;
  ASSERT(sp != NULL);
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
  ASSERT(sp != NULL);
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
  ASSERT(mutex != NULL);
  delete mutex;
  KL_TRC_EXIT;
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = (klib_mutex *)Handle;
  unsigned long wait = Timeout;
  ACPI_STATUS retval;
  SYNC_ACQ_RESULT res;
  ASSERT(mutex != NULL);

  if (wait = 0xFFFF)
  {
    wait = MUTEX_MAX_WAIT;
  }

  res = klib_synch_mutex_acquire(*mutex, wait);

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

void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
  KL_TRC_ENTRY;
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != NULL);

  klib_synch_mutex_release(*mutex);
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

/*void *AcpiOsAllocateZeroed(ACPI_SIZE Size)
{

}*/

void AcpiOsFree(void * Memory)
{
  KL_TRC_ENTRY;
  ASSERT(Memory != NULL);
  delete (char *)Memory;
  KL_TRC_EXIT;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length)
{
  KL_TRC_ENTRY;
  unsigned long start_of_page;
  unsigned long offset;
  unsigned long total_length;
  unsigned int num_pages;
  unsigned long start_of_range;

  offset = (unsigned long)Where % MEM_PAGE_SIZE;
  start_of_page = (unsigned long)Where - offset;
  total_length = (unsigned long)Length + offset;
  num_pages = (total_length / MEM_PAGE_SIZE) + 1;

  start_of_range = (unsigned long)mem_allocate_virtual_range(num_pages);
  mem_map_range((void *)start_of_page, (void *)start_of_range, num_pages);
  start_of_range += offset;
  KL_TRC_EXIT;

  return (void *)start_of_range;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size)
{
  KL_TRC_ENTRY;
  unsigned long addr;
  unsigned long start_of_page;
  unsigned long offset;
  unsigned long total_length;
  unsigned long num_pages;

  addr = (unsigned long)LogicalAddress;
  offset = addr % MEM_PAGE_SIZE;
  start_of_page = addr - offset;
  total_length = Size + offset;
  num_pages = (total_length / MEM_PAGE_SIZE) + 1;

  mem_unmap_range((void *)start_of_page, num_pages);
  mem_deallocate_virtual_range((void *)start_of_page, num_pages);
  KL_TRC_EXIT;
}

// TODO: This capability doesn't exist in the memory manager yet. It may be necessary to add it.
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(AcpiOsGetPhysicalAddress);
  KL_TRC_EXIT;

  return AE_OK;
}

/*
 * Interrupt handlers
 */
// TODO: We don't need these just yet.
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(AcpiOsInstallInterruptHandler);
  KL_TRC_EXIT;
  return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(AcpiOsRemoveInterruptHandler);
  KL_TRC_EXIT;
  return AE_OK;
}

/*
 * Threads and Scheduling
 */
ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
  unsigned long thread_id;
  KL_TRC_ENTRY;
  thread_id = (unsigned long)task_get_cur_thread();

  // If task_get_cur_thread returns NULL then we're still single threaded, in which case it's acceptable to send any
  // positive integer back to ACPICA.
  if (thread_id == 0)
  {
    thread_id = 1;
  }
  return thread_id;
  KL_TRC_EXIT;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
  KL_TRC_ENTRY;
  INCOMPLETE_CODE(AcpiOsExecute);
  KL_TRC_EXIT;
  return AE_OK;
}

// TODO: Don't know quite what this does...
void AcpiOsWaitEventsComplete(void)
{
  KL_TRC_ENTRY;
  panic("AcpiOsWaitEventsComplete - wtf??");
  KL_TRC_EXIT;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
  KL_TRC_ENTRY;

  unsigned long wait_in_ns = Milliseconds * 1000000;

  KL_TRC_DATA("ACPI requests sleep (ns)", wait_in_ns);
  time_sleep_process(wait_in_ns);
  KL_TRC_EXIT;
}

void AcpiOsStall(UINT32 Microseconds)
{
  KL_TRC_ENTRY;
  unsigned long wait_in_ns = Microseconds * 1000;

  KL_TRC_DATA("ACPI requests stall (ns)", wait_in_ns);
  time_stall_process(wait_in_ns);

  KL_TRC_EXIT;
}

/*
 * Platform and hardware-independent I/O interfaces
 */
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Address", Address);
  KL_TRC_DATA("Output address", (unsigned long)Value);
  KL_TRC_DATA("Width", Width);

  ASSERT(Value != (UINT32 *)NULL);
  ASSERT((Width == 8) || (Width == 16) || (Width == 32));

  *Value = proc_read_port(Address, Width);

  KL_TRC_DATA("Value returned", *Value);

  KL_TRC_EXIT;
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
  KL_TRC_ENTRY;

  KL_TRC_DATA("Address", Address);
  KL_TRC_DATA("Value", Value);
  KL_TRC_DATA("Width", Width);

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
  unsigned long *mem = (unsigned long *)AcpiOsMapMemory(Address, Width / 8);
  *Value = *mem;
  AcpiOsUnmapMemory((void *)mem, Width / 8);
  KL_TRC_EXIT;

  return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
  KL_TRC_ENTRY;
  unsigned long *mem = (unsigned long *)AcpiOsMapMemory(Address, Width / 8);

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
  KL_TRC_ENTRY;
  panic("ACPI attempted to read PCI config");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to write PCI config");
  KL_TRC_EXIT;
  return AE_NOT_IMPLEMENTED;
}

/*
 * Miscellaneous
 */
// TODO: This might not always be true in future...
BOOLEAN AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
  return TRUE;
}

// TODO: Might not be true in future...
BOOLEAN AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
  KL_TRC_ENTRY;
  KL_TRC_EXIT;
  return TRUE;
}

UINT64 AcpiOsGetTimer(void)
{
  KL_TRC_ENTRY;
  panic("AcpiOsGetTimer - don't know what this does!");
  KL_TRC_EXIT;
  return 0;
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
  KL_TRC_ENTRY;
  va_list args;

  va_start(args, Format);
  AcpiOsVprintf(Format, args);
  va_end(args);

  KL_TRC_EXIT;
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
  KL_TRC_ENTRY;

  klib_vsnprintf(exception_message_buf, em_buf_len, Format, Args);

  panic(exception_message_buf);
  KL_TRC_EXIT;
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
  return NULL;
}

char *AcpiOsGetNextFilename(void *DirHandle)
{
  KL_TRC_ENTRY;
  panic("ACPI attempted to enumerate directory");
  KL_TRC_EXIT;
  return (char *)NULL;
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
  return NULL;
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
