/// @file
/// @brief Public virtio device interfaces

#pragma once

#include <stdint.h>
#include <list>
#include <vector>
#include "../pci/pci.h"
#include "../pci/generic_device/pci_generic_device.h"

namespace virtio
{
const uint16_t VENDOR_ID = 0x1AF4; ///< Vendor ID used by all virtio devices.

/// @brief Possible virtio device IDs.
enum class DEV_ID : uint16_t
{
  // Transitional device IDs.
  TRANS_NETWORK = 0x1000, ///< Transitional network device.
  TRANS_BLOCK = 0x1001, ///< Transitional block device.
  TRANS_MEM_BALLOON = 0x1002, ///< Transitional memory balloon.
  TRANS_CONSOLE = 0x1003, ///< Transitional console.
  TRANS_SCSI_HOST = 0x1004, ///< Transitional SCSI host.
  TRANS_ENTROPY_SOURCE = 0x1005, ///< Transitional entropy source.
  TRANS_9P_TRANSPORT = 0x1009, ///< Transitional Plan 9 Protocol Transport.

  // New device IDs.
  NET_CARD = 0x1041, ///< Network card.
  BLOCK = 0x1042, ///< Block device.
  CONSOLE = 0x1043, ///< Console.
  ENTROPY_SOURCE = 0x1044, ///< Entropy source.
  TRAD_MEM_BALLOON = 0x1045, ///< Traditional memory balloon.
  IO_MEMORY = 0x1046, ///< IO Memory device.
  RPMSG = 0x1047, ///< 'rpmsg'.
  SCSI_HOST = 0x1048, ///< SCSI host.
  NINE_P_TRANSPORT = 0x1049, ///< Plan 9 Protocol Transport.
  MAC80211_WLAN = 0x104A, ///< mac80211 wlan device.
  RPROC_SERIAL = 0x104B, ///< rproc serial.
  VIRTIO_CAIF = 0x104C, ///< virtio CAIF.
  NEW_MEM_BALLOON = 0x104D, ///< Memory balloon (new style).
  GPU = 0x1050, ///< GPU.
  TIMER_OR_CLOCK = 0x1051, ///< Timer or clock type device.
  INPUT_DEV = 0x1052, ///< Input device.
  SOCKET_DEV = 0x1053, ///< Socket device.
  CRYPTO_DEV = 0x1054,  ///< Crypto device.
  SIGNAL_DIST_MODULE = 0x1055, ///< Signal distribution module.
  PSTORE_DEVICE = 0x1056,  ///< pStore device.
  IOMMU = 0x1057,  ///< IOMMU.
  MEMORY_DEV = 0x1058, ///< Memory device.
};

/// @brief Values that could be given in pci_cap::cfg_type.
///
enum class CONFIG_STRUCTURE_TYPES : uint8_t
{
  COMMON_CFG = 1, ///< Common configuration.
  NOTIFY_CFG = 2, ///< Notifications.
  ISR_CFG = 3, ///< ISR Status.
  DEVICE_CFG = 4, ///< Device-specific configuration.
  PCI_CFG = 5, ///< PCI configuration.
};

/// @brief Virtio defined device status bits.
namespace OPER_STATUS_BITS
{
  const uint8_t ACKNOWLEDGE = 1; ///< Acknowledge the presence of the device.
  const uint8_t DRIVER = 2; ///< The driver knows how to drive this device.
  const uint8_t FAILED = 128; ///< The driver or device has failed.
  const uint8_t FEATURES_OK = 8; ///< The driver and device agree on a set of features.
  const uint8_t DRIVER_OK = 4; ///< The driver is running.
  const uint8_t DEVICE_NEEDS_RESET = 64; ///< The device signals that it needs a reset.
};

/// @brief Feature bits defined in the virtio spec.
///
/// These are further defined in various parts of the virtio spec.
namespace FEATURES
{
  // Generic feature bits for all device types.
  const uint64_t VIRTIO_F_RING_INDIRECT_DESC = (1ULL << 28); ///< Supports rings with indirect descriptors.
  const uint64_t VIRTIO_F_RING_EVENT_IDX = (1ULL << 29); ///< Enables used_event and avail_event flags.
  const uint64_t VIRTIO_F_VERSION_1 = (1ULL << 32); ///< Supports version 1 of the virtio spec.
  const uint64_t VIRTIO_F_ACCESS_PLATFORM = (1ULL << 33); ///< Supports devices with limited/restricted memory access.
  const uint64_t VIRTIO_F_RING_PACKED = (1ULL << 34); ///< Supports the packed ring format.
  const uint64_t VIRTIO_F_IN_ORDER = (1ULL << 35); ///< Buffers are used in the order they're made available.
  const uint64_t VIRTIO_F_ORDER_PLATFORM = (1ULL << 36); ///< Memory accesses are ordered as-per platform.
  const uint64_t VIRTIO_F_SR_IOV = (1ULL << 37); ///< Supports 'single root I/O virtualisation'.
  const uint64_t VIRTIO_F_NOTIFICATION_DATA = (1ULL << 38); ///< Sends extra data in its notifications.

  // Block device feature bits.
  const uint64_t VIRTIO_BLK_F_SIZE_MAX = (1ULL << 1); ///< Maximum size of any single segment is in size_max.
  const uint64_t VIRTIO_BLK_F_SEG_MAX = (1ULL << 2); ///< Maximum number of segments in a request is in seg_max.
  const uint64_t VIRTIO_BLK_F_GEOMETRY = (1ULL << 4); ///< Disk-style geometry specified in geometry.
  const uint64_t VIRTIO_BLK_F_RO = (1ULL << 5); ///< Device is read-only.
  const uint64_t VIRTIO_BLK_F_BLK_SIZE = (1ULL << 6); ///< Block size of disk is in blk_size.
  const uint64_t VIRTIO_BLK_F_FLUSH = (1ULL << 9); ///< Cache flush command support.
  const uint64_t VIRTIO_BLK_F_TOPOLOGY = (1ULL << 10); ///< Device exports information on optimal I/O alignment.
  const uint64_t VIRTIO_BLK_F_CONFIG_WCE = (1ULL << 11); ///< Device cache supports writeback and writethrough modes.
  const uint64_t VIRTIO_BLK_F_DISCARD = (1ULL << 13); ///< Device can support discard command.
  const uint64_t VIRTIO_BLK_F_WRITE_ZEROES = (1ULL << 14); ///< Device supports write zeroes command.
};

/// @brief Structure of PCI capabilities area for virtio configuration.
///
/// This structure comes directly from the virtio specification.
struct pci_cap
{
  uint8_t cap_vendor; ///< Generic PCI field: PCI_CAP_ID_VNDR
  uint8_t cap_next; ///< Generic PCI field: next ptr.
  uint8_t cap_len; ///< Generic PCI field: capability length

  /// Identifies the structure. If NOTIFY_CFG or PCI_CFG, the fields after this one are not relevant. In both cases the
  /// relevant field immediately follows this structure.
  uint8_t cfg_type;

  uint8_t bar; ///< Which BAR points to the referenced config structure.
  uint8_t padding[3]; ///< Pad to full dword.
  uint32_t offset; ///< Offset of the config structure from the address of BAR.
  uint32_t length; ///< Length of the config structure, in bytes.
};
static_assert(sizeof(pci_cap) == 16, "Check sizeof virtio::pci_cap");

/// @brief Virtio configuration common to all devices with PCI transport.
struct pci_common_cfg
{
  volatile uint32_t device_feature_select; ///< Select which set of device_feature bits to access. read-write.
  volatile uint32_t device_feature; ///< Device feature bits. read-only.
  volatile uint32_t driver_feature_select; ///< Select which set of driver_feature bits to access. read-write.
  volatile uint32_t driver_feature; ///< Driver-selected feature bits. read-write.
  volatile uint16_t msix_config; ///< MSI-X configuration. Not used in Azalea. read-write.
  volatile uint16_t num_queues; ///< Number of queues used by this device. read-only.
  volatile uint8_t device_status; ///< Device status. Uses bits from OPER_STATUS_BITS. read-write.
  volatile uint8_t config_generation; ///< Configuration gen number, changed when device updates config. read-only.

  /* About a specific virtqueue. */
  volatile uint16_t queue_select; ///< Which queue do the following fields refer to. read-write.
  volatile uint16_t queue_size; ///< Size of queue in number of entries. read-write.
  volatile uint16_t queue_msix_vector; ///< Queue MSI-X vector number. Not used in Azalea. read-write.
  volatile uint16_t queue_enable; ///< Queue enable flag. read-write.
  volatile uint16_t queue_notify_off; ///< Queue notification address offset. read-only.
  volatile uint64_t queue_desc; ///< Physical address of queue descriptor area. read-write.
  volatile uint64_t queue_driver; ///< Physical address of the available ring - the 'driver area'. read-write.
  volatile uint64_t queue_device; ///< Physical address of the used ring - the 'device area'. read-write.
};
static_assert(sizeof(pci_common_cfg) == 56, "Check sizeof virtio::pci_common_cfg");

/// @brief Flags for queue_descriptor::flags.
namespace Q_DESC_FLAGS
{
  const uint16_t NEXT = 1; ///< Chains with the descriptor in NEXT.
  const uint16_t WRITE = 2; ///< Mark descriptor as device write-only (if not set, descriptor is device read-only)
  const uint16_t INDIRECT = 4; ///< Buffer contains a list of buffer descriptors.
};

/// @brief virtio virtqueue descriptor.
///
struct queue_descriptor
{
  uint64_t phys_addr; ///< Physical address of the buffer.
  uint32_t length; ///< Length of buffer.
  uint16_t flags; ///< Combination of flags from Q_DESC_FLAGS.
  uint16_t next; ///< If (flags & NEXT) then index of the next descriptor.
};
static_assert(sizeof(queue_descriptor) == 16, "Check sizeof virtio::queue_descriptor");

/// @brief Descriptor written by the device to the used ring to describe which descriptors have been used.
struct used_ring_element
{
  uint32_t used_element_idx; ///< The ID of the descriptor that has been used.
  uint32_t length_written; ///< The number of bytes written into the descriptor's buffers.
};

/// @brief Base type for virtio devices to use to store request information in.
class generic_request
{
public:
  virtual ~generic_request() = default;
};

/// @brief Known type of descriptor.
///
/// Virtio requests can broadly be split into 3 parts:
///
/// - The request part. This contains details like what the request is, and how long it is.
/// - Optionally, a buffer to read or write from.
/// - A status byte written by the device.
///
/// These values represent those parts.
enum class descriptor_type
{
  REQUEST = 0, ///< This descriptor represents the request part.
  BUFFER = 1, ///< This descriptor represents the buffer part.
  STATUS = 2, ///< This descriptor represents the status part.
};

/// @brief Descriptor given to virtqueue::send_buffers() to describe the buffers being sent.
///
/// This is a kernel structure used for passing data around the driver, not a virtio structure.
struct buffer_descriptor
{
  void *buffer; ///< Address of the buffer.
  uint32_t buffer_length; ///< Length of this buffer.
  bool device_writable; ///< Is this buffer writable by the device.

  /// Descriptors generally form part of a request. This value allows the caller to correlate returned descriptors to
  /// their parent request. It is otherwise opaque to the queueing system or the rest of the driver.
  std::shared_ptr<generic_request> parent_request;

  /// An opaque value helping the caller to correlate responses to the request they are part of.
  uint64_t request_index{0};

  /// Was this buffer actually handled by the virtio device? This could be false if, e.g., the descriptor couldn't be
  /// enqueued.
  bool handled{false};

  /// What type of descriptor is this?
  descriptor_type type{descriptor_type::REQUEST};
};

std::shared_ptr<pci_generic_device> instantiate_virtio_device(std::shared_ptr<IDevice> parent,
                                                              pci_address dev_addr,
                                                              uint16_t vendor_id,
                                                              uint16_t device_id);

class generic_device;

/// @brief Virtqueue management object
class virtqueue
{
public:
  virtqueue(generic_device *owner_dev, uint16_t size, uint16_t number);
  virtqueue(const virtqueue &) = delete;
  virtqueue(virtqueue &&other);
  virtqueue() = delete;
  ~virtqueue();
  virtqueue &operator=(const virtqueue &) = delete;

  bool send_buffers(std::unique_ptr<buffer_descriptor[]> descriptors, uint16_t num_descriptors);
  void process_used_ring();

  uint64_t descriptor_phys;
  uint64_t avail_ring_phys;
  uint64_t used_ring_phys;

protected:
  // This can be a pointer, since the owner object will destroy this queue on shutdown, there's no possibility of a
  // dangling pointer being left.
  generic_device *owner; ///< Owner device.

  uint16_t queue_number; ///< The index of this particular queue.

  uint16_t queue_size; ///< Number of entries in virtqueue.

  // This section contains the pointers to the various parts of a virtqueue, in the order they appear in memory.
  // They will be populated from a specially mapped memory page.
  queue_descriptor *descriptor_table; ///< The main queue descriptor.

  // Available ring pointers:
  uint16_t *avail_ring_flags; ///< Available ring flags
  uint16_t *avail_ring_idx; ///< Index of the last entry added to the ring.
  uint16_t *avail_ring; ///< Available ring entries.
  uint16_t *avail_ring_used_event; ///< Not used in Azalea.

  // Used ring pointers:
  uint16_t *used_ring_flags; ///< Used ring flags
  uint16_t *used_ring_idx; ///< Index of most recently added element of the used ring.
  used_ring_element *used_ring; ///< Used ring entries.
  uint16_t *used_ring_avail_event; ///< Not used in Azalea.

  ipc::spinlock queue_lock; ///< Lock protecting queue elements.

  uint16_t last_used_ring_idx{0}; ///< Where the device will write the next element in the used ring.

  /// Allows mapping from descriptor index in the queue back to the descriptor provided by the surrounding driver.
  std::vector<buffer_descriptor> buffer_descriptors;
};

/// @brief Generic virtio device.
///
/// This driver does nothing by itself, it must be subclassed to be useful.
class generic_device : public pci_generic_device
{
public:
  generic_device(pci_address address, std::string human_name, std::string dev_name);
  generic_device(const generic_device &) = delete;
  generic_device(generic_device &&) = delete;
  virtual ~generic_device();
  generic_device &operator=(const generic_device &) = delete;

protected:
  friend class virtqueue;

  pci_common_cfg *common_cfg{nullptr}; ///< Pointer to the common virtio configuration block in RAM.

  /// Pointer to the device-specific virtio configuration block in RAM. Child classes will probably cast this to a
  /// suitable type for their class.
  void *device_cfg_void{nullptr};

  /// Pages that were mapped during initial set up to allow access to configuration tables.
  std::list<uint64_t> mapped_phys_addrs;

  std::vector<virtqueue> queues; ///< All virtqueues used by the device.

  // The following can't be a pointer to a specific type, because notify_offset_mult may not be the same as that type's
  // size.
  void *notification_base_addr; ///< Base address for signalling notifications.
  uint32_t notify_offset_mult{0}; ///< Multiplier for calculating notification addresses.

  uint16_t interrupt_number{0}; ///< The 'classic' PCI interrupt number for this device.
  uint32_t *isr_status_byte; ///< The offset to the ISR status byte in the PCI config space.

  // Setup functions
  virtual bool read_pci_config();
  virtual bool map_config_block(pci_cap cap, void **config_block);

  virtual uint64_t read_feature_bits();
  virtual void write_feature_bits(uint64_t new_features);
  virtual bool negotiate_features(uint64_t required_on,
                                  uint64_t required_off,
                                  uint64_t optional_on,
                                  uint64_t optional_off);
  virtual bool configure_interrupts();
  virtual void set_driver_ok();

  virtual void enable_queues();
  virtual void disable_queues();
  virtual void empty_avail_queue();
  virtual void notify_avail_buffers(uint16_t queue_number, uint16_t next_index);

  /// @brief Called by a virtqueue to indicate buffers previously sent have been used and can be released.
  ///
  /// @param desc The descriptor containing the buffer to release. The descriptor itself is still owned by the queue
  ///             and is destroyed later.
  ///
  /// @param bytes_written The number of bytes written to this buffer.
  virtual void release_used_buffer(buffer_descriptor &desc, uint32_t bytes_written) = 0;

  // Overrides of pci_generic_device:
  virtual bool handle_translated_interrupt_fast(uint8_t interrupt_offset,
                                                uint8_t raw_interrupt_num) override;
  virtual void handle_translated_interrupt_slow(uint8_t interrupt_offset,
                                                uint8_t raw_interrupt_num) override;
};

}
