/// @file
/// @brief Virtio 'virtqueue' implementation.

//#define ENABLE_TRACING

#include "virtio.h"
#include "klib/klib.h"

#include <mutex>

namespace virtio
{

/// @brief Standard virtqueue constructor
///
/// @param owner_dev virtio owner device.
///
/// @param size The number of entries in this virtqueue.
///
/// @param number The index of this queue in the owner device.
virtqueue::virtqueue(generic_device *owner_dev, uint16_t size, uint16_t number) :
  owner{owner_dev}, queue_number{number}, queue_size{size}
{
  uint64_t v_addr;
  KL_TRC_ENTRY;

  ASSERT(owner);

  // Allocate memory for the whole queue in one go. It'll then be on one page, and easily aligned.
  descriptor_table = reinterpret_cast<queue_descriptor *>(malloc(MEM_PAGE_SIZE));
  memset(descriptor_table, 0, MEM_PAGE_SIZE);
  v_addr = reinterpret_cast<uint64_t>(descriptor_table);
  ASSERT(descriptor_table);

  // This type of allocation requires placement new operations for the various elements of the queue.
  descriptor_table = new(descriptor_table) queue_descriptor;

  // Then compute the addresses of the rest of the queue.
  v_addr += (sizeof(queue_descriptor) * size);
  avail_ring_flags = reinterpret_cast<uint16_t *>(v_addr);

  v_addr += 2;
  avail_ring_idx = reinterpret_cast<uint16_t *>(v_addr);

  v_addr += 2;
  avail_ring = reinterpret_cast<uint16_t *>(v_addr);

  v_addr += (2 * size);
  avail_ring_used_event = reinterpret_cast<uint16_t *>(v_addr);

  // Used ring pointers:
  v_addr += 2;
  used_ring_flags = reinterpret_cast<uint16_t *>(v_addr);

  v_addr += 2;
  used_ring_idx = reinterpret_cast<uint16_t *>(v_addr);

  v_addr += 2;
  used_ring = reinterpret_cast<used_ring_element *>(v_addr);
  used_ring = new(used_ring) used_ring_element[size];

  v_addr += (sizeof(used_ring_element) * size);
  used_ring_avail_event = reinterpret_cast<uint16_t *>(v_addr);

  descriptor_phys = reinterpret_cast<uint64_t>(mem_get_phys_addr(descriptor_table));
  avail_ring_phys = reinterpret_cast<uint64_t>(mem_get_phys_addr(avail_ring_flags));
  used_ring_phys = reinterpret_cast<uint64_t>(mem_get_phys_addr(used_ring_flags));

  buffer_virtual_addresses = std::unique_ptr<void *[]>(new void *[queue_size]);

  KL_TRC_EXIT;
}

/// @cond
#define MOVE_VAL(x) x = other.x; other.x = nullptr;
/// @endcond

/// @brief Standard move constructor
///
/// @param other The virtqueue to move in to this one.
virtqueue::virtqueue(virtqueue &&other)
{
  KL_TRC_ENTRY;

  owner = other.owner;
  other.owner = nullptr;
  ASSERT(owner);

  queue_size = other.queue_size;
  other.queue_size = 0;

  MOVE_VAL(descriptor_table);
  ASSERT(descriptor_table);

  // Avail ring pointers:
  MOVE_VAL(avail_ring_flags);
  MOVE_VAL(avail_ring_idx);
  MOVE_VAL(avail_ring);
  MOVE_VAL(avail_ring_used_event);

  // Used ring pointers:
  MOVE_VAL(used_ring_flags);
  MOVE_VAL(used_ring_idx);
  MOVE_VAL(used_ring);
  MOVE_VAL(used_ring_avail_event);

  descriptor_phys = other.descriptor_phys;
  avail_ring_phys = other.avail_ring_phys;
  used_ring_phys = other.used_ring_phys;

  other.buffer_virtual_addresses.swap(buffer_virtual_addresses);

  KL_TRC_EXIT;
}

virtqueue::~virtqueue()
{
  KL_TRC_ENTRY;

  if (descriptor_table != nullptr)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Free descriptor table\n");
    free(descriptor_table);
  }

  KL_TRC_EXIT;
}

/// @brief Send a buffer to the device via the available ring.
///
/// At this point, the virtqueue takes ownership of this buffer. It releases ownership back to the previous owner when
/// the device has put it back on the used queue, or when this queue is destroyed.
///
/// @param descriptors Set of buffers to send to the device
///
/// @param num_descriptors Number of descriptors in 'descriptors'
///
/// @return true if the buffer was successfully added to the avail queue. False if the avail queue is already full.
bool virtqueue::send_buffers(std::unique_ptr<buffer_descriptor[]> &descriptors, uint16_t num_descriptors)
{
  bool result{true};
  uint16_t cur_ring_idx;

  KL_TRC_ENTRY;

  if (num_descriptors == 0)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "num_descriptors must be > 0\n");
    result = false;
  }
  else
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Valid number of descriptors.\n");
    std::scoped_lock<kernel_spinlock_obj> guard(queue_lock);
    std::unique_ptr<uint16_t[]> indicies = std::unique_ptr<uint16_t[]>(new uint16_t[num_descriptors]);
    uint16_t found_so_far{0};

    // Steps are taken directly from section 2.6.13 of the virtio spec "Supplying buffers to the device".

    // 1. The driver places the buffer into free descriptor(s) in the descriptor table, chaining as necessary (see
    ///   2.6.5 The Virtqueue Descriptor Table).
    // This is a very naive algorithm, it'll do for now. Search for empty descriptor slots.

    for (uint16_t i = 0; i < queue_size; i++)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Examine index ", i, "\n");

      if (descriptor_table[i].phys_addr == 0)
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Found empty entry at index ", i, "\n");

        indicies[found_so_far] = i;

        found_so_far++;
        if (found_so_far == num_descriptors)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Found sufficient slots\n");
          break;
        }
      }
    }

    if (found_so_far == num_descriptors)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Found enough slots, start populating them.\n");
      for (uint16_t j = 0; j < num_descriptors; j++)
      {
        uint16_t this_index = indicies[j];
        KL_TRC_TRACE(TRC_LVL::FLOW, "Use index ", this_index, "\n");
        descriptor_table[this_index].phys_addr = reinterpret_cast<uint64_t>(mem_get_phys_addr(descriptors[j].buffer));
        descriptor_table[this_index].flags = descriptors[j].device_writable ? Q_DESC_FLAGS::WRITE : 0;

        buffer_virtual_addresses[this_index] = descriptors[j].buffer;

        if (j < (num_descriptors - 1))
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Chain to next index as well.\n");
          descriptor_table[this_index].flags |= Q_DESC_FLAGS::NEXT;
          descriptor_table[this_index].next = indicies[j + 1];
        }
        else
        {
          descriptor_table[this_index].next = 0;
        }
        descriptor_table[this_index].length = descriptors[j].buffer_length;
      }

      // 2. The driver places the index of the head of the descriptor chain into the next ring entry of the available
      //    ring.
      cur_ring_idx = *avail_ring_idx;
      avail_ring[cur_ring_idx % queue_size] = indicies[0];

      // 3. Steps 1 and 2 MAY be performed repeatedly if batching is possible.
      // 4. The driver performs a suitable memory barrier to ensure the device sees the updated descriptor table and
      //    available ring before the next step.
      std::atomic_thread_fence(std::memory_order_seq_cst);

      // 5. The available idx is increased by the number of descriptor chain heads added to the available ring.
      cur_ring_idx++;
      *avail_ring_idx = cur_ring_idx;

      // 6. The driver performs a suitable memory barrier to ensure that it updates the idx field before checking for
      //    notification suppression.
      std::atomic_thread_fence(std::memory_order_seq_cst);

      // 7. The driver sends an available buffer notification to the device if such notifications are not suppressed.
      //    (We do not suppress notifications yet)
      owner->notify_avail_buffers(queue_number, cur_ring_idx);
    }
    else
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Unable to find sufficient space\n");
      result = false;
    }
  }

  KL_TRC_TRACE(TRC_LVL::EXTRA, "Result: ", result, "\n");
  KL_TRC_EXIT;

  return result;
}

/// @brief Release used elements of the used ring.
///
void virtqueue::process_used_ring()
{
  std::scoped_lock<kernel_spinlock_obj> guard(queue_lock);
  uint32_t bytes_left;
  uint16_t descriptor_index;
  uint32_t bytes_this_buffer;

  KL_TRC_ENTRY;

  while(*used_ring_idx != last_used_ring_idx)
  {
    KL_TRC_TRACE(TRC_LVL::FLOW, "Process used ring element ", last_used_ring_idx, "\n");

    bytes_left = used_ring[last_used_ring_idx].length_written;

    descriptor_index = used_ring[last_used_ring_idx].used_element_idx;
    while(1)
    {
      KL_TRC_TRACE(TRC_LVL::FLOW, "Descriptor to free: ", descriptor_index, "\n");

      if (descriptor_table[descriptor_index].flags & Q_DESC_FLAGS::WRITE)
      {
        bytes_this_buffer = descriptor_table[descriptor_index].length;
        if (bytes_this_buffer > bytes_left)
        {
          KL_TRC_TRACE(TRC_LVL::FLOW, "Write truncated\n");
          bytes_this_buffer = bytes_left;
        }

        bytes_left -= bytes_this_buffer;
      }
      else
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "Was a read only buffer, no bytes written\n");
        bytes_this_buffer = 0;
      }

      owner->release_used_buffer(buffer_virtual_addresses[descriptor_index], bytes_this_buffer);
      descriptor_table[descriptor_index].phys_addr = 0;

      if (!(descriptor_table[descriptor_index].flags & Q_DESC_FLAGS::NEXT))
      {
        KL_TRC_TRACE(TRC_LVL::FLOW, "No more descriptors to retire\n");
        break;
      }

      descriptor_index = descriptor_table[descriptor_index].next;
    }

    last_used_ring_idx++;
  }

  KL_TRC_EXIT;
}

};
