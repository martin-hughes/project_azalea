/// @file
/// @brief Define xHCI context structures.

#pragma once

#include <stdint.h>

namespace usb { namespace xhci
{

#pragma pack(push, 1)

  // This namespace isn't usefully documented.
  /// @cond
  ///
  namespace EP_TYPES
  {
    const uint8_t INVALID = 0;
    const uint8_t ISOCH_OUT = 1;
    const uint8_t BULK_OUT = 2;
    const uint8_t INTERRUPT_OUT = 3;
    const uint8_t CONTROL = 4;
    const uint8_t ISOCH_IN = 5;
    const uint8_t BULK_IN = 6;
    const uint8_t INTERRUPT_IN = 7;
  };
  /// @endcond

  // This namespace isn't usefully documented.
  /// @cond
  ///
  namespace EP_DOORBELL_CODE
  {
    const uint8_t RESERVED = 0;
    const uint8_t CONTROL_EP_0 = 1;
    const uint8_t EP_1_OUT = 2;
    const uint8_t EP_1_IN = 3;
    const uint8_t EP_2_OUT = 4;
    const uint8_t EP_2_IN = 5;
    const uint8_t EP_3_OUT = 6;
    const uint8_t EP_3_IN = 7;
    const uint8_t EP_4_OUT = 8;
    const uint8_t EP_4_IN = 9;
    const uint8_t EP_5_OUT = 10;
    const uint8_t EP_5_IN = 11;
    const uint8_t EP_6_OUT = 12;
    const uint8_t EP_6_IN = 13;
    const uint8_t EP_7_OUT = 14;
    const uint8_t EP_7_IN = 15;
    const uint8_t EP_8_OUT = 16;
    const uint8_t EP_8_IN = 17;
    const uint8_t EP_9_OUT = 18;
    const uint8_t EP_9_IN = 19;
    const uint8_t EP_10_OUT = 20;
    const uint8_t EP_10_IN = 21;
    const uint8_t EP_11_OUT = 22;
    const uint8_t EP_11_IN = 23;
    const uint8_t EP_12_OUT = 24;
    const uint8_t EP_12_IN = 25;
    const uint8_t EP_13_OUT = 26;
    const uint8_t EP_13_IN = 27;
    const uint8_t EP_14_OUT = 28;
    const uint8_t EP_14_IN = 29;
    const uint8_t EP_15_OUT = 30;
    const uint8_t EP_15_IN = 31;
  }
  /// @endcond

  /// @brief An xHCI Slot Context structure.
  ///
  /// See the xHCI specification section 6.2.2 ("Slot Context") for more information.
  struct slot_context
  {
    /// @cond
    uint32_t route_string : 20;
    uint32_t speed : 4;
    uint32_t reserved_1 : 1;
    uint32_t multi_tt : 1;
    uint32_t is_hub : 1;
    uint32_t num_context_entries : 5;

    uint32_t max_exit_latency : 16;
    uint32_t root_hub_port_number : 8;
    uint32_t number_downstream_ports : 8;

    uint32_t tt_hub_slot_id : 8;
    uint32_t tt_port_number : 8;
    uint32_t tt_think_time : 2;
    uint32_t reserved_2 : 4;
    uint32_t interrupter_num : 10;

    uint32_t usb_device_addr : 8;
    uint32_t reserved_3 : 19;
    uint32_t slot_state : 5;

    uint32_t reserved_4;
    uint32_t reserved_5;
    uint32_t reserved_6;
    uint32_t reserved_7;
    /// @endcond
  };
  static_assert(sizeof(slot_context) == 32, "Sizeof slot context wrong.");

  /// @brief An xHCI endpoint context
  ///
  /// See the xHCI specification section 6.2.3 ("Endpoint Context") for more information.
  struct endpoint_context
  {
    /// @cond
    uint32_t endpoint_state : 3;
    uint32_t reserved_1 : 5;
    uint32_t mult : 2;
    uint32_t max_primary_streams : 5;
    uint32_t linear_stream_array : 1;
    uint32_t interval : 8;
    uint32_t max_esit_payload_hi : 8;

    uint32_t reserved_2 : 1;
    uint32_t error_count : 2;
    uint32_t endpoint_type : 3;
    uint32_t reserved_3 : 1;
    uint32_t host_initiate_disable : 1;
    uint32_t max_burst_size : 8;
    uint32_t max_packet_size : 16;

    union
    {
      struct
      {
        uint64_t dequeue_cycle_state : 1;
        uint64_t reserved_4 : 63;
      };
      uint64_t tr_dequeue_phys_ptr;
    };

    uint16_t average_trb_length;
    uint16_t max_esit_payload_lo;

    uint32_t reserved_5;
    uint32_t reserved_6;
    uint32_t reserved_7;
    /// @endcond
  };
  static_assert(sizeof(endpoint_context) == 32, "Sizeof endpoint context wrong.");

  /// @brief An xHCI Input Control Context structure.
  ///
  /// See the xHCI specification section 6.2.5.1 ("Input Control Context") for more information.
  struct input_control_context
  {
    /// @cond
    uint32_t drop_context_flags;
    uint32_t add_context_flags;

    uint32_t reserved_1;
    uint32_t reserved_2;
    uint32_t reserved_3;
    uint32_t reserved_4;
    uint32_t reserved_5;

    uint32_t config_value : 8;
    uint32_t interface_number : 8;
    uint32_t alternate_setting : 8;
    uint32_t reserved_6 : 8;
    /// @endcond
  };
  static_assert(sizeof(input_control_context) == 32, "Sizeof input control context wrong.");

  /// @brief An xHCI Device Context structure.
  ///
  /// See the xHCI specification section 6.2.1 ("Device Context") for more information.
  struct device_context
  {
    /// @cond
    slot_context slot;
    endpoint_context ep_0_bi_dir;
    struct
    {
      endpoint_context out;
      endpoint_context in;
    } endpoints[15];
    /// @endcond
  };
  static_assert(sizeof(device_context) == 1024, "Sizeof device context wrong.");

  /// @brief An xHCI Input Context structure.
  ///
  /// See the xHCI specification section 6.2.5 ("Input Context") for more information.
  struct input_context
  {
    /// @cond
    input_control_context control;
    device_context device;
    /// @endcond
  };
  static_assert(sizeof(input_context) == 1056, "Sizeof input context wrong.");

#pragma pack(pop)

}; };
