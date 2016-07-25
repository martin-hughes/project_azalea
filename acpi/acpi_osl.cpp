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


ACPI_STATUS AcpiOsInitialize(void)
{
  INCOMPLETE_CODE(AcpiOsInitlialize);

  return AE_OK ;
}

ACPI_STATUS AcpiOsTerminate(void)
{
  panic("Hit AcpiOsTerminate - should never be called.");

  return AE_OK;
}

/*
 * ACPI Table interfaces
 */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
  ACPI_PHYSICAL_ADDRESS  Ret;
  Ret = 0;
  AcpiFindRootPointer(&Ret);
  return Ret;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal)
{
  *NewVal = (ACPI_STRING)NULL;

  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
  *NewTable = (ACPI_TABLE_HEADER *)NULL;

  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength)
{
  *NewAddress = (ACPI_PHYSICAL_ADDRESS)NULL;

  return AE_OK;
}

/*
 * Spinlock primitives
 */
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
  kernel_spinlock *lock = new kernel_spinlock;
  klib_synch_spinlock_init(*lock);
  *OutHandle = (ACPI_SPINLOCK)lock;

  return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != NULL);
  delete lock;
}

// The ACPI_CPU_FLAGS return value is simply passed to the AcpiOsReleaseLock function, so can be ignored.
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != NULL);
  klib_synch_spinlock_lock(*lock);

  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
  kernel_spinlock *lock = (kernel_spinlock *)Handle;
  ASSERT(lock != NULL);
  klib_synch_spinlock_unlock(*lock);
}

/*
 * Semaphore primitives
 */
ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
  klib_semaphore *sp = new klib_semaphore;
  klib_synch_semaphore_init(*sp, MaxUnits, InitialUnits);
  *OutHandle = (ACPI_SEMAPHORE)sp;

  return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
  klib_semaphore *sp = (klib_semaphore *)Handle;
  ASSERT(sp != NULL);
  delete sp;

  return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
  klib_semaphore *sp = (klib_semaphore *)Handle;
  ASSERT(sp != NULL);
  ASSERT(Units == 1);

  klib_synch_semaphore_wait(*sp, Timeout);

  return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
  klib_semaphore *sp = (klib_semaphore *)Handle;
  ASSERT(sp != NULL);
  ASSERT(Units == 1);

  klib_synch_semaphore_clear(*sp);

  return AE_OK;
}

/*
 * Mutex primitives. May be configured to use semaphores instead via
 * ACPI_MUTEX_TYPE (see platform/acenv.h)
 */
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
{
  klib_mutex *mutex = new klib_mutex;
  klib_synch_mutex_init(*mutex);
  *OutHandle = (ACPI_MUTEX)mutex;

  return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != NULL);
  delete mutex;
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != NULL);

  klib_synch_mutex_acquire(*mutex, Timeout);

  return AE_OK;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
  klib_mutex *mutex = (klib_mutex *)Handle;
  ASSERT(mutex != NULL);

  klib_synch_mutex_release(*mutex);
}

/*
 * Memory allocation and mapping
 */
void *AcpiOsAllocate(ACPI_SIZE Size)
{
  return (void *)(new char[Size]);
}

/*void *AcpiOsAllocateZeroed(ACPI_SIZE Size)
{

}*/

void AcpiOsFree(void * Memory)
{
  ASSERT(Memory != NULL);
  delete (char *)Memory;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length)
{
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

  return (void *)start_of_range;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size)
{
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
}

// TODO: This capability doesn't exist in the memory manager yet. It may be necessary to add it.
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
  INCOMPLETE_CODE(AcpiOsGetPhysicalAddress);

  return AE_OK;
}

/*
 * Memory/Object Cache. These shouldn't be used by ACPICA, as there's a flag set in ac64bit.h telling it to use it's
 * own cache.
 */
ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize, UINT16 MaxDepth,
                              ACPI_CACHE_T **ReturnCache)
{
  panic("ACPI attempted to create cache");
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache)
{
  panic("ACPI attempted to delete cache");
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache)
{
  panic("ACPI attempted to purge cache");
  return AE_NOT_IMPLEMENTED;
}

void *AcpiOsAcquireObject(ACPI_CACHE_T *Cache)
{
  panic("ACPI attempted to acquire object");
  return NULL;
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object)
{
  panic("ACPI attempted to release object");
  return AE_NOT_IMPLEMENTED;
}

/*
 * Interrupt handlers
 */
// TODO: We don't need these just yet.
ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
  INCOMPLETE_CODE(AcpiOsInstallInterruptHandler);
  return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine)
{
  INCOMPLETE_CODE(AcpiOsRemoveInterruptHandler);
  return AE_OK;
}

/*
 * Threads and Scheduling
 */
ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
  INCOMPLETE_CODE(AcpiOsGetThreadId);
  return AE_OK;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
  INCOMPLETE_CODE(AcpiOsExecute);
  return AE_OK;
}

// TODO: Don't know quite what this does...
void AcpiOsWaitEventsComplete(void)
{
  panic("AcpiOsWaitEventsComplete - wtf??");
}

void AcpiOsSleep(UINT64 Milliseconds)
{
  panic("ACPI attempted sleep");
}

void AcpiOsStall(UINT32 Microseconds)
{
  panic("ACPI attempted stall");
}

/*
 * Platform and hardware-independent I/O interfaces
 */
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
  panic("ACPI OS Read Port not implemented");
  return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
  panic("ACPI OS Write Port not implemented");
  return AE_OK;
}

/*
 * Platform and hardware-independent physical memory interfaces
 */
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
  unsigned long *mem = (unsigned long *)AcpiOsMapMemory(Address, Width / 8);
  *Value = *mem;
  AcpiOsUnmapMemory((void *)mem, Width / 8);

  return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
  unsigned long *mem = (unsigned long *)AcpiOsMapMemory(Address, Width / 8);

  kl_memcpy(&Value, (void *)mem, Width / 8);

  AcpiOsUnmapMemory((void *)mem, Width / 8);

  return AE_OK;
}

/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 *Value, UINT32 Width)
{
  panic("ACPI attempted to read PCI config");
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
  panic("ACPI attempted to write PCI config");
  return AE_NOT_IMPLEMENTED;
}

/*
 * Miscellaneous
 */
// TODO: This might not always be true in future...
BOOLEAN AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
  return TRUE;
}

// TODO: Might not be true in future...
BOOLEAN AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
  return TRUE;
}

UINT64 AcpiOsGetTimer(void)
{
  panic("AcpiOsGetTimer - don't know what this does!");
  return 0;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
  panic("ACPI attempted to signal function");
  return AE_NOT_IMPLEMENTED;
}

/*
 * Debug print routines
 */
void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...)
{
  panic("ACPI attempted printf");
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
  panic("ACPI attempted vprintf");
}

void AcpiOsRedirectOutput(void *Destination)
{
  panic("ACPI attempted output change");
}

/*
 * Debug input
 */
ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
  panic("ACPI attempted to read keyboard");
  return AE_OK;
}

/*
 * Obtain ACPI table(s)
 */
ACPI_STATUS AcpiOsGetTableByName(char *Signature, UINT32 Instance, ACPI_TABLE_HEADER **Table, ACPI_PHYSICAL_ADDRESS *Address)
{
  panic("Attempting to fetch table by name");
  return AE_NO_ACPI_TABLES;
}

ACPI_STATUS AcpiOsGetTableByIndex(UINT32 Index, ACPI_TABLE_HEADER **Table, UINT32 *Instance, ACPI_PHYSICAL_ADDRESS *Address)
{
  panic("Attempting to fetch table by index");
  return AE_NO_ACPI_TABLES;
}

ACPI_STATUS AcpiOsGetTableByAddress(ACPI_PHYSICAL_ADDRESS Address, ACPI_TABLE_HEADER **Table)
{
  panic("Attempting to fetch table by address");
  return AE_NO_ACPI_TABLES;
}

/*
 * Directory manipulation
 */
void *AcpiOsOpenDirectory(char *Pathname, char *WildcardSpec, char RequestedFileType)
{
  panic("ACPI attempted to open directory");
  return NULL;
}

char *AcpiOsGetNextFilename(void *DirHandle)
{
  panic("ACPI attempted to enumerate directory");
  return (char *)NULL;
}

void AcpiOsCloseDirectory(void *DirHandle)
{
  panic("ACPI attempted to close directory");
}

/*
 * File I/O and related support
 */
ACPI_FILE AcpiOsOpenFile (const char *Path, UINT8 Modes)
{
  panic("ACPI attempted to open file");
  return NULL;
}

void AcpiOsCloseFile(ACPI_FILE File)
{
  panic("ACPI attempted to close file");
}

int AcpiOsReadFile(ACPI_FILE File, void *Buffer, ACPI_SIZE Size, ACPI_SIZE Count)
{
  panic("ACPI attempted to read file");
  return AE_NOT_IMPLEMENTED;
}

int AcpiOsWriteFile(ACPI_FILE File, void *Buffer, ACPI_SIZE Size, ACPI_SIZE Count)
{
  panic("ACPI attempted to write file");
  return AE_NOT_IMPLEMENTED;
}

long AcpiOsGetFileOffset(ACPI_FILE File)
{
  panic("ACPI attempted to to find file offset");
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsSetFileOffset(ACPI_FILE File, long Offset, UINT8 From)
{
  panic("ACPI attempted to set file offset");
  return AE_NOT_IMPLEMENTED;
}

void AcpiOsTracePoint(ACPI_TRACE_EVENT_TYPE Type, BOOLEAN Begin, UINT8 *Aml, char *Pathname)
{
  panic("ACPI trace point called");
}
