/// @file
/// @brief Kernel memory allocator.
///
/// It is expected that most kernel memory allocation requests will come through these functions. Exceptions would be
/// allocations that require an explicit mapping between physical and virtual addresses. Functions that simply need
/// new/delete type allocations should call through here.
///
/// The functions kmalloc/kfree and their associates use a modified slab allocation system. Memory requests are
/// categorised in to different "chunk sizes", where the possible chunk sizes are given in the CHUNK_SIZES list, and
/// where the assigned chunk size is larger than the requested amount of memory.
///
/// Requests for chunks larger than the maximum chunk size are allocated entire pages.
///
/// Each different chunk size is fulfilled from a slab of memory items of that size. Each slab consists of a data area,
/// followed by as many chunks as will fit (aligned) into the remaining space. The slabs then record which chunks are
/// allocated, and which are free.
///
/// To simplify searching for a free chunk, slabs are categorized as "empty", "full", or "partly full". When looking
/// for a free chunk, the "partly full" slabs are used first, followed by empty slabs. If there are no empty or partly
/// full slabs available, a new slab is allocated. If a slab becomes empty, it is added to the empty slabs list. If the
/// empty slabs list exceeds a certain length (MAX_EMPTY_SLABS) the mostly recently emptied slab is deallocated.
///
/// Each slab has the following basic format:
///
/// {
///   klib_list_item<void *> - used to store the slab in the fullness lists.
///   unsigned long - Stores the number of allocated items
///   unsigned long[] - Stores a bitmap indicating which items are full with a 1.
///   items - Aligned to the correct size, stores the items from this chunk.
/// }
///

//#define ENABLE_TRACING

#include "klib/data_structures/lists.h"
#include "memory.h"
#include "klib/c_helpers/buffers.h"
#include "klib/panic/panic.h"
#include "klib/misc/assert.h"
#include "klib/tracing/tracing.h"
#include "klib/synch/kernel_locks.h"
#include "klib/synch/kernel_mutexes.h"

typedef klib_list<void *> PTR_LIST;
typedef klib_list_item<void *> PTR_LIST_ITEM;
struct slab_header
{
  PTR_LIST_ITEM list_entry;
  uint64_t allocation_count;
};

// The assertion below ensures that the size of slab_header hasn't changed. If it does, the number of available chunks
// and their offsets within the NUM_CHUNKS_PER_SLAB and FIRST_OFFSET_IN_SLAB will need updating.
static_assert(sizeof(slab_header) == 40,
              "If this assert fails, make sure to re-run chunk_sizer.py and update memory.cpp");

//------------------------------------------------------------------------------
// Allocator control variables. The chunk sizes and offsets are calculated by
// hand, based on the header being 40 bytes, 1 bit per bitmap entry with the
// bitmap growing by 8 bytes at a time, and the first chunk being aligned with
// its own size. Chunk sizes must be a power of two.
//
// (There's a chunk_sizer.py script in /build_support that can help with this.)
//------------------------------------------------------------------------------
namespace
{
  const uint32_t CHUNK_SIZES[] = {8, 64, 256, 1024, 262144};
  const uint32_t NUM_CHUNKS_PER_SLAB[] = {258041, 32703, 8187, 2047, 7};
  const uint32_t FIRST_OFFSET_IN_SLAB[] = {32824, 4160, 1280, 1024, 262144};
  const uint32_t NUM_SLAB_LISTS = (sizeof(CHUNK_SIZES) / sizeof(CHUNK_SIZES[0]));
  const uint32_t MAX_CHUNK_SIZE = CHUNK_SIZES[NUM_SLAB_LISTS - 1];
  const uint32_t FIRST_BITMAP_ENTRY_OFFSET = 40;
  const uint32_t MAX_FREE_SLABS = 5;

  // This is currently redundant since the addition of the mutex system, below. It remains in place to (hopefully!)
  // simplify a removal of the mutex in a later update of the allocator.
  kernel_spinlock slabs_list_lock;
  PTR_LIST free_slabs_list[NUM_SLAB_LISTS];
  PTR_LIST partial_slabs_list[NUM_SLAB_LISTS];
  PTR_LIST full_slabs_list[NUM_SLAB_LISTS];

  // Allowing two threads to run kmalloc or kfree at once is a bad idea - the code is not thread safe. As a simple, and
  // hopefully temporary, fix we put a mutex around kmalloc and kfree. A normal spinlock is insufficient, since the
  // called function trees of both kmalloc and kfree include both kmalloc and kfree.
  klib_mutex allocator_gen_lock;

  bool allocator_initialized = false;
  bool allocator_initializing = false;
}

static_assert(FIRST_BITMAP_ENTRY_OFFSET == sizeof(slab_header),
              "Make sure you have correctly set up chunk sizes, etc.");

//------------------------------------------------------------------------------
// Helper function declarations.
//------------------------------------------------------------------------------
void init_allocator_system();
void *allocate_new_slab(uint32_t chunk_size_idx);
void *allocate_chunk_from_slab(void *slab, uint32_t chunk_size_idx);
bool slab_is_full(void* slab, uint32_t chunk_size_idx);
bool slab_is_empty(void* slab, uint32_t chunk_size_idx);

//------------------------------------------------------------------------------
// Main malloc & free functions.
//------------------------------------------------------------------------------

/// @brief Drop-in replacement for malloc that allocates memory for use within the kernel.
///
/// Kernel's malloc function. Operates just like the normal malloc. The allocated memory is guaranteed to be within the
/// kernel's virtual memory space. If there is no spare memory, the system will panic.
///
/// Operation is as per the file description for memory.cpp.
///
/// @param mem_size The number of bytes required.
///
/// @return A pointer to the newly allocated memory.
void *kmalloc(uint64_t mem_size)
{
  KL_TRC_ENTRY;

  void *return_addr;
  void *slab_ptr;
  slab_header *slab_header_ptr;
  uint32_t required_pages;
  uint64_t proportion_used;
  SYNC_ACQ_RESULT res;
  bool release_mutex_at_end = true;

  // Make sure the one-time-only initialisation of the system is complete. This set of ifs and asserts isn't meant to
  // provide full thread safety, instead it is meant to prevent any accidental circular recursion starting.
  if (!allocator_initialized)
  {
    ASSERT(!allocator_initializing);
    init_allocator_system();
    ASSERT(allocator_initialized);
  }

  res = klib_synch_mutex_acquire(allocator_gen_lock, MUTEX_MAX_WAIT);
  ASSERT((res == SYNC_ACQ_ACQUIRED) || (res == SYNC_ACQ_ALREADY_OWNED));
  if (res == SYNC_ACQ_ALREADY_OWNED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Don't release mutex\n");
    release_mutex_at_end = false;
  }

  // Figure out the index of all the chunk lists to use.
  uint32_t slab_idx = NUM_SLAB_LISTS;
  for(uint32_t i = 0; i < NUM_SLAB_LISTS; i++)
  {
    if (mem_size <= CHUNK_SIZES[i])
    {
      slab_idx = i;
      break;
    }
  }

  // If the requested RAM is larger than we support via chunks, do a large malloc.
  if (slab_idx >= NUM_SLAB_LISTS)
  {
    required_pages = ((mem_size - 1) / MEM_PAGE_SIZE) + 1;
    if (release_mutex_at_end)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing mutex from kmalloc 1\n");
      klib_synch_mutex_release(allocator_gen_lock, false);
    }
    KL_TRC_TRACE(TRC_LVL::FLOW, "Big allocation. Pages needed", required_pages, "\n");
    KL_TRC_EXIT;
    return mem_allocate_pages(required_pages);
  }

  // Find or allocate a suitable slab to use. Use partially full slabs first - this prevents there being lots of only
  // partially-used slabs. If there isn't a partially full slab to use then pick up the next empty one. If there aren't
  // any of those then allocate a new slab.
  //
  // In this choosing process we keep a lock, and then remove the chosen slab from the lists before freeing the lock.
  // This prevents two threads choosing the same slab and both attempting to allocate the last remaining item from it.
  // If a second thread finds no remaining slabs in any list, it will simply allocate a new one. This leads to some
  // extra slabs being used.
  klib_synch_spinlock_lock(slabs_list_lock);
  if(!klib_list_is_empty(&partial_slabs_list[slab_idx]))
  {
    // Use one of the partially empty slabs
    slab_ptr = partial_slabs_list[slab_idx].head;
    slab_header_ptr = (slab_header *)slab_ptr;

    klib_list_remove(&slab_header_ptr->list_entry);
    klib_synch_spinlock_unlock(slabs_list_lock);
  }
  else if (!klib_list_is_empty(&free_slabs_list[slab_idx]))
  {
    // Get the first totally empty slab
    slab_ptr = free_slabs_list[slab_idx].head;
    slab_header_ptr = (slab_header *)slab_ptr;

    klib_list_remove(&slab_header_ptr->list_entry);
    klib_synch_spinlock_unlock(slabs_list_lock);
  }
  else
  {
    // No slabs free, so allocate a new slab.
    klib_synch_spinlock_unlock(slabs_list_lock);
    slab_ptr = allocate_new_slab(slab_idx);
    slab_header_ptr = (slab_header *)slab_ptr;
  }

  return_addr = allocate_chunk_from_slab(slab_ptr, slab_idx);
  ASSERT(return_addr != nullptr);

  // If the slab is completely full, add it to the appropriate list. If it isn't, it must be at least partially full
  // now, so add it to that list.
  klib_synch_spinlock_lock(slabs_list_lock);
  if (slab_is_full(slab_ptr, slab_idx))
  {
    klib_list_add_head(&full_slabs_list[slab_idx], &slab_header_ptr->list_entry);
  }
  else
  {
    klib_list_add_head(&partial_slabs_list[slab_idx], &slab_header_ptr->list_entry);
  }
  klib_synch_spinlock_unlock(slabs_list_lock);

  // If this slab is more than 90% full and there aren't any spare empty slabs
  // left, pre-allocate one now.
  //
  // This is a (hopefully) temporary solution to the following problem: if the
  // VMM requires a new list item, it will call this code to generate one. But
  // if there are no slabs available for use, this code will call back to the
  // VMM For more pages, leading to an infinite loop of allocations.
  // Do this entirely in integers to avoid having to write floating point code.
  proportion_used = (slab_header_ptr->allocation_count * 100) /
      NUM_CHUNKS_PER_SLAB[slab_idx];
  if ((proportion_used > 90) && klib_list_is_empty(&free_slabs_list[slab_idx]))
  {
    slab_ptr = allocate_new_slab(slab_idx);
    slab_header_ptr = (slab_header *)slab_ptr;
    klib_synch_spinlock_lock(slabs_list_lock);
    klib_list_add_head(&free_slabs_list[slab_idx], &slab_header_ptr->list_entry);
    klib_synch_spinlock_unlock(slabs_list_lock);
  }

  if (release_mutex_at_end)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing mutex from kmalloc 2\n");
    klib_synch_mutex_release(allocator_gen_lock, false);
  }

  KL_TRC_EXIT;

  return return_addr;
}

/// @brief Kernel memory deallocator
///
/// Drop in replacement for free() that frees memory from kmalloc().
///
/// @param mem_block The memory to be freed.
void kfree(void *mem_block)
{
  KL_TRC_ENTRY;

  uint64_t mem_ptr_num = reinterpret_cast<uint64_t>(mem_block);
  slab_header *slab_ptr;
  klib_list<void *> *list_ptr;
  uint64_t list_ptr_base_num_a;
  uint64_t list_ptr_base_num_b;
  uint32_t chunk_size_idx;
  const uint64_t list_array_size = sizeof(free_slabs_list);
  bool slab_was_full = false;
  uint32_t chunk_offset;
  uint64_t *bitmap_ptr;
  uint32_t bitmap_ulong;
  uint32_t bitmap_bit;
  uint64_t bitmap_mask;
  uint64_t free_slabs;
  bool release_mutex_at_end = true;
  SYNC_ACQ_RESULT res;

  ASSERT(allocator_initialized);

  res = klib_synch_mutex_acquire(allocator_gen_lock, MUTEX_MAX_WAIT);
  ASSERT((res == SYNC_ACQ_ACQUIRED) || (res == SYNC_ACQ_ALREADY_OWNED));
  if (res == SYNC_ACQ_ALREADY_OWNED)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Don't release mutex\n");
    release_mutex_at_end = false;
  }

  // First, decide whether this is a "large allocation" or not. If it's a large allocation, the address being freed
  // will lie on a memory page boundary.
  if (mem_ptr_num % MEM_PAGE_SIZE == 0)
  {
    // This is a large allocation, which still needs to be properly implemented.
    panic ("Large allocation support not complete.");
  }
  else
  {
    // Figure out which slab this chunk comes from.
    slab_ptr = (slab_header *)(mem_ptr_num - (mem_ptr_num % MEM_PAGE_SIZE));

    // See which list this slab is in to help figure out the size of the chunk.
    list_ptr = slab_ptr->list_entry.list_obj;

    // Is this in one of the partially-full slab lists?
    mem_ptr_num = (uint64_t)list_ptr;
    list_ptr_base_num_a = (uint64_t)(&partial_slabs_list[0]);
    list_ptr_base_num_b = (uint64_t)(&full_slabs_list[0]);
    if ((mem_ptr_num >= list_ptr_base_num_a) &&
        (mem_ptr_num < (list_ptr_base_num_a + list_array_size)))
    {
      // Partially full slab.
      chunk_size_idx = (mem_ptr_num - list_ptr_base_num_a) / sizeof(PTR_LIST);
    }
    else if ((mem_ptr_num >= list_ptr_base_num_b) &&
             (mem_ptr_num < (list_ptr_base_num_b + list_array_size)))
    {
      // Full slab. Make a note that this slab is no longer full. Later on, when we've deallocated the relevant chunk,
      // and the slab is actually partially full, it can be moved to the partially full list.
      chunk_size_idx = (mem_ptr_num - list_ptr_base_num_b) / sizeof(PTR_LIST);
      slab_was_full = true;
    }
    else
    {
      // Slab isn't in a recognised list. There's not a lot we can do - memory
      // has already been corrupted, so bail out.
      ASSERT(0);
    }

    // Calculate how many chunks after the first chunk we are.
    ASSERT(chunk_size_idx < NUM_SLAB_LISTS);
    chunk_offset = (uint64_t)mem_block - (uint64_t)slab_ptr;
    chunk_offset = chunk_offset - FIRST_OFFSET_IN_SLAB[chunk_size_idx];
    chunk_offset = chunk_offset / CHUNK_SIZES[chunk_size_idx];
    ASSERT(chunk_offset < NUM_CHUNKS_PER_SLAB[chunk_size_idx]);

    // Figure out which ulong to look at, and the offset within that.
    bitmap_ulong = chunk_offset / 64;
    bitmap_bit = 63 - (chunk_offset % 64);

    // Clear that bit from the allocation bit mask.
    bitmap_mask = (uint64_t)1 << bitmap_bit;
    bitmap_ptr = (uint64_t *)(((uint64_t)slab_ptr) + FIRST_BITMAP_ENTRY_OFFSET);
    bitmap_ptr += bitmap_ulong;
    ASSERT((*bitmap_ptr & bitmap_mask) != 0);
    *bitmap_ptr = *bitmap_ptr ^ bitmap_mask;

    // Decrement the count of chunks allocated from this slab. If the slab is
    // empty, add it to the list of empty slabs or get rid of it, as appropriate
    slab_ptr->allocation_count = slab_ptr->allocation_count - 1;
    if (slab_is_empty(slab_ptr, chunk_size_idx))
    {
      klib_synch_spinlock_lock(slabs_list_lock);
      klib_list_remove(&slab_ptr->list_entry);
      klib_synch_spinlock_unlock(slabs_list_lock);
      free_slabs = klib_list_get_length(&free_slabs_list[chunk_size_idx]);
      if (free_slabs >= MAX_FREE_SLABS)
      {
        mem_deallocate_pages(slab_ptr, 1);
      }
      else
      {
        klib_synch_spinlock_lock(slabs_list_lock);
        klib_list_add_tail(&free_slabs_list[chunk_size_idx], &slab_ptr->list_entry);
        klib_synch_spinlock_unlock(slabs_list_lock);
      }
    }
    else if(slab_was_full)
    {
      klib_synch_spinlock_lock(slabs_list_lock);
      klib_list_remove(&slab_ptr->list_entry);
      klib_list_add_tail(&partial_slabs_list[chunk_size_idx], &slab_ptr->list_entry);
      klib_synch_spinlock_unlock(slabs_list_lock);
    }
  }

  if (release_mutex_at_end)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Releasing mutex from kfree\n");
    klib_synch_mutex_release(allocator_gen_lock, false);
  }

  KL_TRC_EXIT;
}

//------------------------------------------------------------------------------
// Helper function definitions.
//------------------------------------------------------------------------------

/// @brief Initialize the Kernel's kmalloc/kfree system.
///
/// One time initialisation of the allocator system. **Must only be called once**.
void init_allocator_system()
{
  KL_TRC_ENTRY;

  void *new_empty_slab;
  slab_header *new_empty_slab_header;

  ASSERT(!allocator_initialized);
  ASSERT(!allocator_initializing);

  static_assert(sizeof(CHUNK_SIZES) == sizeof(NUM_CHUNKS_PER_SLAB),
                "MMGR mismatch - CHUNK_SIZES and NUM_CHUNKS_PER_SLAB arrays don't correspond.");
  static_assert(sizeof(CHUNK_SIZES) == sizeof(FIRST_OFFSET_IN_SLAB),
                "MMGR mismatch - CHUNK_SIZES and FIRST_OFFSET_IN_SLAB arrays don't correspond.");
  static_assert(sizeof(slab_header) <= FIRST_BITMAP_ENTRY_OFFSET,
                "MMGR mismatch - The slab header would scribble the first allocatable area.");

  allocator_initializing = true;

  // Initialise the slab lists.
  //
  // It's not enough to simply initialise these lists, because once someone calls kmalloc that function will try to
  // kmalloc a new list item, which will lead to an infinite loop. Therefore, create one empty slab of each size and
  // add it to the empty lists now. This means that the first call of kmalloc is guaranteed to be able to find a slab
  // to create list entries in.
  for(uint32_t i = 0; i < NUM_SLAB_LISTS; i++)
  {
    klib_list_initialize(&free_slabs_list[i]);
    klib_list_initialize(&partial_slabs_list[i]);
    klib_list_initialize(&full_slabs_list[i]);

    new_empty_slab = allocate_new_slab(i);
    ASSERT(new_empty_slab != nullptr);
    new_empty_slab_header = (slab_header *)new_empty_slab;
    klib_list_add_tail(&free_slabs_list[i], &new_empty_slab_header->list_entry);
  }

  klib_synch_spinlock_init(slabs_list_lock);
  klib_synch_mutex_init(allocator_gen_lock);

  allocator_initialized = true;
  allocator_initializing = false;

  KL_TRC_EXIT;
}

/// @brief Allocate a new slab for kmalloc/kfree.
///
/// Allocate and initialise a new slab. Don't add it to any slab lists - that is the caller's responsibility.
///
/// @param chunk_size_idx The index into CHUNK_SIZES that specifies how big the chunks used in this slab are.
///
/// @return The address of a new slab
void *allocate_new_slab(uint32_t chunk_size_idx)
{
  KL_TRC_ENTRY;

  uint32_t bitmap_bytes;
  char *slab_buffer;

  // Allocate a new slab and fill in the header.
  void *new_slab = mem_allocate_pages(1);
  slab_header* new_slab_header = (slab_header *)new_slab;
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Got address: ", new_slab_header, "\n");
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Got address 2: ", (&new_slab_header->list_entry), "\n");
  klib_list_item_initialize(&new_slab_header->list_entry);
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "List initialized.\n");
  new_slab_header->list_entry.item = new_slab;
  new_slab_header->allocation_count = 0;
  KL_TRC_TRACE(TRC_LVL::IMPORTANT, "Written to address\n");

  // Empty the allocation bitmap. Round up to the next whole number of 8-byte
  // longs.
  bitmap_bytes = NUM_CHUNKS_PER_SLAB[chunk_size_idx] / 8;
  bitmap_bytes++;
  bitmap_bytes = bitmap_bytes / 8;
  bitmap_bytes++;
  bitmap_bytes = bitmap_bytes * 8;

  slab_buffer = (char *)new_slab;
  slab_buffer = slab_buffer + sizeof(slab_header);
  kl_memset(slab_buffer, 0, bitmap_bytes);

  KL_TRC_EXIT;

  return new_slab;
}

/// @brief Allocate a chunk of the correct size from this slab.
///
/// Using this slab, and given the chunk size of the slab, allocate a new chunk and mark that chunk as in use.
///
/// @param slab The slab to allocate from
///
/// @param chunk_size_idx The index into CHUNK_SIZES that specifies how big the chunk to allocate is.
///
/// @return The address of the newly allocated chunk.
void *allocate_chunk_from_slab(void *slab, uint32_t chunk_size_idx)
{
  KL_TRC_ENTRY;

  uint64_t test_mask = 0xFFFFFFFFFFFFFFFF;
  uint64_t moving_bit_mask;
  uint64_t *test_location;
  uint64_t test_result;
  char *slab_as_bytes;
  slab_header *slab_ptr;
  uint32_t first_free_idx = 0;
  uint64_t chunk_offset;


  ASSERT(slab != nullptr);
  ASSERT(chunk_size_idx < NUM_SLAB_LISTS);

  // Compute the address of the first part of the bitmap.
  slab_ptr = (slab_header *)slab;
  slab_as_bytes = (char *)slab;
  slab_as_bytes += sizeof(slab_header);
  test_location = (uint64_t *)slab_as_bytes;

  // Continue looping until a free spot is found in this slab. If we go past the
  // maximum possible number that means the caller has passed a full slab, which
  // is invalid, so assert.
  while(1)
  {
    test_result = test_mask ^ *test_location;

    // If the result is non-zero, there was at least 1 bit set to zero in the
    // header bytes. Figure out which bit was first, to give us the index.
    if (test_result != 0)
    {
      moving_bit_mask = 0x8000000000000000;

      while(1)
      {
        // If this test is true, then we've found a zero-bit, implying that the
        // chunk it refers to is free. Set this bit to 1 (i.e. allocated) and
        // skip out of these loops.
        if (((*test_location) & moving_bit_mask) == 0)
        {
          (*test_location) = (*test_location) | moving_bit_mask;
          break;
        }
        first_free_idx++;

        // If this assert hits then there has been a programming error, because
        // it implies that all of the bits in the long under test were set to
        // 1, i.e. allocated.
        ASSERT(moving_bit_mask != 1);
        moving_bit_mask >>= 1;
      }

      break;
    }
    else
    {
      test_location++;
      first_free_idx += 64;
    }

    // If this assert hits, the slab was full when it was passed in to this
    // function, which is a violation of the function's interface.
    ASSERT(first_free_idx <= NUM_CHUNKS_PER_SLAB[chunk_size_idx]);
  }

  // If this assert hits, the slab was full when it was passed in to this
  // function, which is a violation of the function's interface.
  ASSERT(first_free_idx <= NUM_CHUNKS_PER_SLAB[chunk_size_idx]);

  // At this point, we've got the index of a free chunk in the slab. All that
  // remains is to convert it into a memory location, which can be passed back
  // to the caller.
  chunk_offset = (first_free_idx * CHUNK_SIZES[chunk_size_idx]) +
                                           FIRST_OFFSET_IN_SLAB[chunk_size_idx];
  slab_as_bytes = (char *)slab;
  slab_as_bytes += chunk_offset;

  slab_ptr->allocation_count = slab_ptr->allocation_count + 1;

  KL_TRC_EXIT;

  return (void *)slab_as_bytes;
}

/// @brief Is the specified slab full?
///
/// Simply tests to see whether the passed slab is full or not.
///
/// @param slab The slab to check
///
/// @param chunk_size_idx The index into CHUNK_SIZES representing the size of chunks within this slab
///
/// @return Whether the slab is full or not.
bool slab_is_full(void* slab, uint32_t chunk_size_idx)
{
  KL_TRC_ENTRY;

  slab_header *slab_header_ptr = (slab_header *)slab;
  uint32_t max_chunks;

  ASSERT(slab != nullptr);
  ASSERT(chunk_size_idx < NUM_SLAB_LISTS);
  max_chunks = NUM_CHUNKS_PER_SLAB[chunk_size_idx];
  ASSERT(slab_header_ptr->allocation_count <= max_chunks);

  KL_TRC_EXIT;

  return (slab_header_ptr->allocation_count == max_chunks);
}

/// @brief Is the specified slab empty or not?
///
/// Simply test to see whether the passed slab is empty or not.
///
/// @param slab The slab to check
///
/// @param chunk_size_idx The index into CHUNK_SIZES representing the size of chunks within this slab
///
/// @return Whether the slab is empty or not.
bool slab_is_empty(void* slab, uint32_t chunk_size_idx)
{
  KL_TRC_ENTRY;

  slab_header *slab_header_ptr = (slab_header *)slab;

  ASSERT(slab != nullptr);

  KL_TRC_EXIT;

  return (slab_header_ptr->allocation_count == 0);
}

/// @brief Reset the memory allocator during testing.
///
/// **This function must only be used in test code.** It is used to reset the allocation system in order to allow a
/// clean set of tests to be carried out. It is absolutely not safe to use in the live system, but it's desirable to
/// expose this single interface rather than allowing the test code to play with the internals of this file directly.
///
/// **Note:** This invalidates any allocations done using kmalloc. Test code must not reuse those allocations after
/// calling this function.
#ifdef AZALEA_TEST_CODE
void test_only_reset_allocator()
{
  KL_TRC_ENTRY;

  if (allocator_initialized)
  {
    slab_header *slab_ptr;

    // Spin through each possible list in turn, removing the slabs from the list
    // and freeing them.
    for (uint32_t i = 0; i < NUM_SLAB_LISTS; i++)
    {
      while(!klib_list_is_empty(&free_slabs_list[i]))
      {
        slab_ptr = (slab_header *)free_slabs_list[i].head->item;
        klib_list_remove(&slab_ptr->list_entry);
        mem_deallocate_pages(slab_ptr, 1);
      }
      while(!klib_list_is_empty(&partial_slabs_list[i]))
      {
        slab_ptr = (slab_header *)partial_slabs_list[i].head->item;
        klib_list_remove(&slab_ptr->list_entry);
        mem_deallocate_pages(slab_ptr, 1);
      }
      while(!klib_list_is_empty(&full_slabs_list[i]))
      {
        slab_ptr = (slab_header *)full_slabs_list[i].head->item;
        klib_list_remove(&slab_ptr->list_entry);
        mem_deallocate_pages(slab_ptr, 1);
      }
    }

    allocator_initialized = false;
  }
  
  KL_TRC_EXIT;
}
#endif
