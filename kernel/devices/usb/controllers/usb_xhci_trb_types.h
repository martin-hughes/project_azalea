/// @file
/// @brief TRB structure definitions.
///
/// More details of each of these structures can be found in the xHCI specification.

#pragma once

#include <stdint.h>
#include <memory>

namespace usb { namespace xhci
{
  /// @brief Numerical constants used in the TRB type field.
  ///
  /// See the spec section 6.4.6 for these constants.
  namespace TRB_TYPES
  {
    const uint8_t RESERVED = 0;                 ///< Reserved for xHCI use.
    const uint8_t NORMAL = 1;                   ///< Normal transfer TRB.
    const uint8_t SETUP_STAGE = 2;              ///< Setup stage transfer TRB.
    const uint8_t DATA_STAGE = 3;               ///< Data stage transfer TRB.
    const uint8_t STATUS_STAGE = 4;             ///< Status stage transfer TRB.
    const uint8_t ISOCH = 5;                    ///< Isoch tranfer TRB.
    const uint8_t LINK = 6;                     ///< Transfer or command ring link TRB.
    const uint8_t EVENT_DATA = 7;               ///< Transfer event data TRB.
    const uint8_t NO_OP = 8;                    ///< Transfer ring no-op.
    const uint8_t ENABLE_SLOT_CMD = 9;          ///< Enable slot command TRB.
    const uint8_t DISABLE_SLOT_CMD = 10;        ///< Disable slot command TRB.
    const uint8_t ADDRESS_DEVICE_CMD = 11;      ///< Address device command TRB.
    const uint8_t CONFIG_ENDPOINT_CMD = 12;     ///< Configure endpoint command TRB.
    const uint8_t EVAL_CONTEXT_CMD = 13;        ///< Evaluate context command TRB.
    const uint8_t RESET_ENDPOINT_CMD = 14;      ///< Reset endpoint command TRB.
    const uint8_t STOP_ENDPOINT_CMD = 15;       ///< Stop endpoint command TRB.
    const uint8_t SET_TR_DEQUEUE_PTR_CMD = 16;  ///< Set Transfer Ring Dequeue pointer command TRB.
    const uint8_t RESET_DEVICE_CMD = 17;        ///< Reset device command TRB.
    const uint8_t FORCE_EVENT_CMD = 18;         ///< Force event command TRB.
    const uint8_t NEGOTIATE_BANDWIDTH_CMD = 19; ///< Negotiate bandwidth command TRB.
    const uint8_t SET_LATENCY_TOL_CMD = 20;     ///< Set latency tolerance command TRB.
    const uint8_t GET_PORT_BANDWIDTH_CMD = 21;  ///< Get port bandwidth command TRB.
    const uint8_t FORCE_HEADER_CMD = 22;        ///< Force header send command TRB.
    const uint8_t NO_OP_CMD = 23;               ///< No-op command TRB.
    const uint8_t TRANSFER_EVENT = 32;          ///< Transfer event TRB.
    const uint8_t COMMAND_COMPLETE_EVENT = 33;  ///< Command completion event TRB.
    const uint8_t PORT_STS_CHANGE_EVENT = 34;   ///< Port status change event TRB.
    const uint8_t BANDWIDTH_REQUEST_EVENT = 35; ///< Bandwidth request event TRB.
    const uint8_t DOORBELL_EVENT = 36;          ///< Doorbell event TRB.
    const uint8_t HOST_CONTROLLER_EVENT = 37;   ///< Host controller event TRB.
    const uint8_t DEVICE_NOTFN_EVENT = 38;      ///< Device notification event TRB.
    const uint8_t MFINDEX_WRAP_EVENT = 39;      ///< MFINDEX counter wrap event TRB.
  };

  /// @brief Numerical values of the completion codes given by command completion TRBs.
  ///
  /// See section 6.4.5 for details.
  namespace C_CODES
  {
    const uint8_t INVALID = 0;                          ///< The completion code hasn't been changed.
    const uint8_t SUCCESS = 1;                          ///< The operation completed successfully.
    const uint8_t DATA_BUFFER_ERR = 2;                  ///< The data buffer wasn't suitable - either over- or underrun
    const uint8_t BABBLE_DETECTED_ERR = 3;              ///< Babbling detected during this operation.
    const uint8_t USB_TRANSACTION_ERR = 4;              ///< The device responded incorrectly.
    const uint8_t TRB_ERR = 5;                          ///< An error condition in a TRB.
    const uint8_t STALL_ERR = 6;                        ///< The USB device has stalled.
    const uint8_t RESOURCE_ERR = 7;                     ///< Insufficient resources to complete the operation.
    const uint8_t BANDWIDTH_ERR = 8;                    ///< Insufficient bandwidth is available for this device.
    const uint8_t NO_SLOTS_AVAIL = 9;                   ///< No slots remain on the controller.
    const uint8_t INVALID_STREAM_TYPE_ERR = 10;         ///< Invalid stream type presented.
    const uint8_t SLOT_NOT_ENABLED_ERR = 11;            ///< A command has been issued to a disabled slot.
    const uint8_t ENDPOINT_NOT_ENABLED_ERR = 12;        ///< A doorbell has been rung for a device that is disabled.
    const uint8_t SHORT_PACKET = 13;                    ///< The number of bytes transferred was less than requested.
    const uint8_t RING_UNDERRUN = 14;                   ///< The transfer ring for an isoch endpoint is empty.
    const uint8_t RING_OVERRUN = 15;                    ///< The transfer ring for an isoch endpoint is full.
    const uint8_t VF_EVENT_RING_FULL_ERR = 16;          ///< A VF's event ring was full during a force event command.
    const uint8_t PARAM_ERR = 17;                       ///< A context parameter is invalid.
    const uint8_t BANDWIDTH_OVERRUN_ERR = 18;           ///< The TD exceeded the bandwidth available to the device.
    const uint8_t CONTEXT_STATE_ERR = 19;               ///< A comand was issued to transition from an illegal state.
    const uint8_t NO_PING_RESPONSE_ERR = 20;            ///< A ping response was not received in time.
    const uint8_t EVENT_RING_FULL_ERR = 21;             ///< The controller attempted to post an event to a full ring.
    const uint8_t INCOMPAT_DEVICE_ERR = 22;             ///< The xHCI has detected an incompatible device.
    const uint8_t MISSED_SERVICE_ERR = 23;              ///< An isoch endpoint wasn't serviced in time.
    const uint8_t CMD_RING_STOPPED = 24;                ///< A command stop operation occurred.
    const uint8_t CMD_ABORTED = 25;                     ///< A command abort operation occurred.
    const uint8_t STOPPED = 26;                         ///< The endpoint was stopped.
    const uint8_t STOPPED_LENGTH_INVALID = 27;          ///< A transfer was stopped and the transfer length is invalid.
    const uint8_t STOPPED_SHORT_PACKET = 28;            ///< A transfer was stopped in short packet conditions.
    const uint8_t MAX_EXIT_LATENCY_TOO_LARGE_ERR = 29;  ///< The proposed max exit latency was too large.
    const uint8_t ISOCH_BUFFER_OVERRUN = 31;            ///< The isoch endpoint needs to send more data than the buffer
    const uint8_t EVENT_LOST_ERR = 32;                  ///< xHCI implementation-specific failure.
    const uint8_t UNDEFINED_ERR = 33;                   ///< An undefined error occurred.
    const uint8_t INVALID_STREAM_ID_ERR = 34;           ///< An invalid stream ID was received.
    const uint8_t SECONDARY_BANDWIDTH_ERR = 35;         ///< Not enough bandwidth could be given to a secondary domain.
    const uint8_t SPLIT_TRANSACTION_ERR = 36;           ///< An error was detected on a USB2 split transaction.
  };

#pragma pack(push, 1)

  //----------------
  // Generic TRBs.
  //----------------

  /// @brief The most basic TRB - an empty one.
  ///
  /// No further documentation is needed. See the xHCI spec for more details.
  struct template_trb
  {
    /// @cond
    union
    {
      struct
      {
        uint64_t reserved_1;  ///< Reserved.
        uint64_t reserved_2;  ///< Reserved.
      };
      struct
      {
        uint64_t data;        ///< Data
        uint32_t status;      ///< Status

        uint32_t cycle : 1;   ///< Usual position of the cycle bit.
        uint32_t evaluate_next_trb : 1; ///< Usual position of the Evaluate Next TRB bit.
        uint32_t reserved_3 : 8;  ///< Often set to reserved.
        uint32_t trb_type : 6;  ///< The TRB type from TRB_TYPES.
        uint32_t reserved_4 : 16; ///< Reserved.
      };
    };

    /// @brief Clear the TRB to zero.
    ///
    void populate()
    {
      reserved_1 = 0;
      reserved_2 = 0;
    }
    /// @endcond
  };
  static_assert(sizeof(template_trb) == 16, "Template TRB size wrong.");

  /// @brief Link TRBs link the ring to the next TRB.
  ///
  /// They are also used at the end of the ring to link back to the first TRB.
  struct link_trb
  {
    uint64_t trb_addr;  ///< The physical address of the next ring segment.
    uint32_t reserved_1 : 22; ///< Reserved.
    uint32_t interrupter : 10;  ///< The interrupter that will receive events from this TRB.
    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t toggle_cycle : 1;  ///< Should the xHC toggle the internal cycle bit state?
    uint32_t reserved_2 : 2;  ///< Reserved.
    uint32_t chain : 1; ///< Chain.
    uint32_t interrupt_on_complete : 1; ///< Interrupt when this TRB completes.
    uint32_t reserved_3 : 4;  ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::LINK
    uint32_t reserved_4 : 16; ///< Reserved.

    /// @brief Create a link TRB.
    ///
    /// @param next_trb_phys_addr The physical address of the next TRB in the ring.
    ///
    /// @param interrupter The interrupter to use for this TRB.
    ///
    /// @param interrupt_complete Interrupt on completion of this TRB?
    ///
    /// @param chain Chain bit
    ///
    /// @param toggle_cycle Should the xHCI toggle its interpretation of the cycle bit?
    ///
    /// @param cycle Cycle bit.
    void populate(void *next_trb_phys_addr,
                  uint16_t interrupter,
                  bool interrupt_complete,
                  bool chain,
                  bool toggle_cycle,
                  bool cycle)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      this->trb_type = TRB_TYPES::LINK;
      this->trb_addr = reinterpret_cast<uint64_t>(next_trb_phys_addr);
      this->interrupter = interrupter;
      this->cycle = cycle;
      this->toggle_cycle = toggle_cycle;
      this->chain = chain;
      this->interrupt_on_complete = interrupt_complete ? 1 : 0;
    };
  };
  static_assert(sizeof(link_trb) == 16, "Link TRB size wrong.");

  //---------------
  // Command TRBs.
  //---------------

  /// @brief A no-op command TRB.
  ///
  struct no_op_cmd_trb
  {
    uint64_t reserved_1;  ///< Reserved.
    uint32_t reserved_2;  ///< Reserved.
    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t reserved_3 : 9;  ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::NO_OP_CMD
    uint32_t reserved_4 : 16; ///< Reserved.

    /// @brief Populate a no_op_cmd_trb
    ///
    /// @param cycle Cycle bit.
    void populate(bool cycle)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      this->cycle = cycle;
      this->trb_type = TRB_TYPES::NO_OP_CMD;
    };
  };
  static_assert(sizeof(no_op_cmd_trb) == 16, "No Op Cmd TRB size wrong.");

  /// @brief An enable slot command TRB.
  ///
  struct enable_slot_cmd_trb
  {
    uint64_t reserved_1;  ///< Reserved.
    uint32_t reserved_2;  ///< Reserved.
    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t reserved_3 : 9;  ///< Reserved.
    uint32_t trb_type : 6;  ///< Set to TRB_TYPES::ENABLE_SLOT_CMD
    uint32_t slot_type : 5; ///< The slot type being requested. An xHC specific value given by the relevant port.
    uint32_t reserved_4 : 11; ///< Reserved.

    /// @brief Populate an enable slot command TRB.
    ///
    /// @param cycle Cycle bit.
    ///
    /// @param slot_type The slot type requested - associated with the port by the xHCI extended capabilities.
    void populate(bool cycle, uint32_t slot_type)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      this->cycle = cycle;
      this->slot_type = slot_type;
      this->trb_type = TRB_TYPES::ENABLE_SLOT_CMD;
    };
  };
  static_assert(sizeof(enable_slot_cmd_trb) == 16, "Enable Slot Cmd TRB size wrong.");

  /// @brief An address device command TRB.
  ///
  struct address_device_cmd_trb
  {
    uint64_t input_context_ptr_phys; ///< Pointer to the input context for the device being configured.
    uint32_t reserved_1; ///< Reserved.
    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t reserved_2 : 8; ///< Reserved.
    uint32_t block_set_address_request : 1; ///< TBD.
    uint32_t trb_type : 6; ///< Will be set to TRB_TYPES::ADDRESS_DEVICE_CMD or TRB_TYPES::EVAL_CONTEXT_CMD.
    uint32_t reserved_3 : 8; ///< Reserved.
    uint32_t slot_id : 8; ///< ID of the slot being configured.

    /// @brief Populate this TRB.
    ///
    /// @param address_device If True, this TRB is being used for an Address Device command. If false, it is being used
    ///                       for an Evaluate Context command.
    ///
    /// @param cycle Should the cycle bit be set.
    ///
    /// @param input_context_phys Physical address of the input context to evaluate / address.
    ///
    /// @param block_set_address See xHCI spec for details.
    ///
    /// @param slot_id The slot ID of the device to address or evaluate.
    void populate(bool address_device,
                  bool cycle,
                  uint64_t input_context_phys,
                  bool block_set_address,
                  uint8_t slot_id)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      this->trb_type = address_device ? TRB_TYPES::ADDRESS_DEVICE_CMD : TRB_TYPES::EVAL_CONTEXT_CMD;
      this->input_context_ptr_phys = input_context_phys;
      this->cycle = cycle;
      this->block_set_address_request = block_set_address;
      this->slot_id = slot_id;
    };
  };
  static_assert(sizeof(address_device_cmd_trb) == 16, "Address Device Cmd TRB size wrong.");

  // The structure of both of these TRBs is the same.
  typedef address_device_cmd_trb evaluate_context_cmd_trb; ///< Identical to the Address Device Command TRB.

  /// @brief A Configure Endpoint command TRB.
  ///
  struct configure_endpoint_cmd_trb
  {
    uint64_t input_context_ptr_phys; ///< Pointer to the input context for the device being configured.
    uint32_t reserved_1; ///< Reserved.
    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t reserved_2 : 8; ///< Reserved.
    uint32_t deconfigure : 1; ///< If 0, configure the endpoint. If 1, deconfigure the endpoint.
    uint32_t trb_type : 6; ///< Will be set to TRB_TYPES::CONFIG_ENDPOINT_CMD
    uint32_t reserved_3 : 8; ///< Reserved.
    uint32_t slot_id : 8; ///< ID of the slot being configured.

    configure_endpoint_cmd_trb() : input_context_ptr_phys{0},
                                   reserved_1{0},
                                   cycle{0},
                                   reserved_2{0},
                                   deconfigure{0},
                                   trb_type{TRB_TYPES::CONFIG_ENDPOINT_CMD},
                                   reserved_3{0},
                                   slot_id{0}
    { };
  };
  static_assert(sizeof(configure_endpoint_cmd_trb) == 16, "Configure Endpoint Cmd TRB size wrong.");

  //--------------
  // Event TRBs.
  //--------------

  /// @brief A port status change event TRB.
  ///
  struct port_status_change_event_trb
  {
    uint8_t reserved_1[3]; ///< Reserved.
    uint8_t port_id; ///< The port that has changed status.
    uint32_t reserved_2; ///< Reserved.
    uint8_t reserved_3[3]; ///< Reserved.
    uint8_t completion_code; ///< Standard completion code - one of C_CODES.
    uint32_t cycle : 1; ///< The cycle bit.
    uint32_t reserved_4 : 9; ///< Reserved.
    uint32_t trb_type : 6; ///< TRB type: should be set to TRB_TYPES::PORT_STS_CHANGE_EVENT
    uint32_t reserved_5 : 16; ///< Reserved.
  };
  static_assert(sizeof(port_status_change_event_trb) == 16, "Port status change event TRB size wrong.");

  /// @brief A command completion event TRB.
  ///
  struct command_completion_event_trb
  {
    uint64_t command_trb_phys_addr; ///< Physical address of the command that generated this TRB.
    uint32_t completion_param : 24; ///< Command specific completion parameter.
    uint32_t completion_code : 8; ///< Standard completion code - one of C_CODES.

    uint32_t cycle : 1; ///< The cycle bit.
    uint32_t reserved : 9; ///< Reserved.
    uint32_t trb_type : 6; ///< TRB type: should be set to TRB_TYPES::COMMAND_COMPLETE_EVENT
    uint32_t vf_id : 8; ///< Virtual function ID of generating controller (not used in Azalea)
    uint32_t slot_id : 8; ///< The slot ID of the device generating this event.
  };
  static_assert(sizeof(command_completion_event_trb) == 16, "Command completion event TRB size wrong.");

  /// @brief A transfer event TRB.
  ///
  struct transfer_event_trb
  {
    uint64_t trb_pointer; ///< Pointer to generating TRB or 64 bits of data.
    uint32_t transfer_length_left : 24; ///< Number of bytes not yet transferred.
    uint32_t completion_code : 8; ///< One of the codes in C_CODES.

    uint32_t cycle : 1; ///< Ring cycle bit.
    uint32_t reserved_1 : 1; ///< Reserved.
    uint32_t event_data : 1; ///< 'Event Data' - if one, trb_pointer actually contains data
    uint32_t reserved_2 : 7; ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::TRANSFER_EVENT
    uint32_t endpoint_id : 5; ///< ID of the generating endpoint.
    uint32_t reserved_3 : 3; ///< Reserved.
    uint32_t slot_id : 8; ///< ID of the slot generating this event.
  };
  static_assert(sizeof(transfer_event_trb) == 16, "Transfer event TRB size wrong.");

  //----------------
  // Transfer TRBs.
  //----------------

  /// @brief A no-op transfer TRB.
  ///
  struct no_op_transfer_trb
  {
    uint64_t reserved_1; ///< Reserved.
    uint32_t reserved_2 : 22; ///< Reserved.
    uint32_t interrupter_target : 10; ///< The interrupter to target once the no-op completes.

    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t evaluate_next_trb : 1; ///< See xHCI spec for details.
    uint32_t reserved_3 : 2; ///< Reserved.
    uint32_t chain : 1; ///< Chain this TRB with the next one.
    uint32_t interrupt_on_complete : 1; ///< Interrupt once no-op complete.
    uint32_t reserved_4 : 4; ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::NO_OP
    uint32_t reserved_5 : 16; ///< Reserved.

    /// @brief Populate the No-op transfer TRB.
    ///
    /// @param int_target The target interrupter.
    ///
    /// @param cycle The cycle bit
    ///
    /// @param ent Evaluate Next TRB.
    ///
    /// @param chain The Chain bit.
    ///
    /// @param ioc Interrupt on Completion.
    void populate(uint16_t int_target, bool cycle, bool ent, bool chain, bool ioc)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      reserved_5 = 0;

      this->cycle = cycle;
      this->evaluate_next_trb = ent;
      this->chain = chain;
      this->interrupt_on_complete = ioc;
      this->interrupter_target = int_target;
      this->trb_type = TRB_TYPES::NO_OP;
    };
  };
  static_assert(sizeof(no_op_transfer_trb) == 16, "No Op Transfer TRB size wrong.");

  /// @brief A normal transfer TRB.
  ///
  struct normal_transfer_trb
  {
    uint64_t data_buffer_ptr_phys; ///< Pointer to the data buffer (or immediate data)
    uint32_t transfer_length : 17; ///< The number of bytes to transfer.
    uint32_t td_size : 5; ///< The number of packets in this transfer.
    uint32_t interrupter_target : 10; ///< The target interrupter.

    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t evaluate_next_trb : 1; ///< See xHCI spec for details.
    uint32_t interrupt_on_short_pkt : 1; ///< Interrupt if a short packet received.
    uint32_t no_snoop : 1; ///< Set PCI no snoop. See xHCI spec.
    uint32_t chain : 1; ///< Chain bit.
    uint32_t interrupt_on_complete : 1; ///< Interrupt when transfer complete.
    uint32_t immediate_data : 1; ///< Immediate data in 'data_buffer_ptr_phys' instead of pointer.
    uint32_t reserved_1 : 2; ///< Reserved.
    uint32_t block_event_interrupt : 1; ///< If this and interrupt_on_complete set, do not interrupt at next threshold.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::NORMAL
    uint32_t reserved_2 : 16; ///< Reserved.

    /// @brief Populate a normal transfer TRB.
    ///
    /// @param data_buffer_phys Either a physical pointer to the send/receive data buffer in memory, or, if
    ///                         "immediate_data" is set, storage for up to 8 bytes of data.
    ///
    /// @param transfer_len The number of bytes to transfer.
    ///
    /// @param td_size The expected number of packets in the transfer.
    ///
    /// @param interrupter The target interrupter on completion.
    ///
    /// @param cycle The cycle bit.
    ///
    /// @param evaluate_next_trb Evaluate the next TRB before completing this transfer - see the xHCI spec for more
    ///                          details.
    ///
    /// @param interrupt_on_short_pkt If the device returns a short packet, interrupt anyway with a suitable completion
    ///                               code.
    ///
    /// @param no_snoop Set a no snoop attribute on the PCI bus - see the xHCI spec for more details.
    ///
    /// @param chain The chain bit.
    ///
    /// @param interrupt_on_complete Trigger an interrupt on the target interrupter once the transfer is complete.
    ///
    /// @param immediate_data If set, data_buffer_phys is used as storage for up to 8 bytes of data instead of being a
    ///                       pointer to a buffer in RAM.
    ///
    /// @param block_event_interrupt If this and interrupt_on_complete set, do not interrupt at next threshold.
    void populate(uint64_t data_buffer_phys,
                  uint32_t transfer_len,
                  uint16_t td_size,
                  uint16_t interrupter,
                  bool cycle,
                  bool evaluate_next_trb,
                  bool interrupt_on_short_pkt,
                  bool no_snoop,
                  bool chain,
                  bool interrupt_on_complete,
                  bool immediate_data,
                  bool block_event_interrupt)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      this->trb_type = TRB_TYPES::NORMAL;

      this->data_buffer_ptr_phys = data_buffer_phys;
      this->transfer_length = transfer_len;
      this->td_size = td_size;
      this->interrupter_target = interrupter;
      this->cycle = cycle;
      this->evaluate_next_trb = evaluate_next_trb;
      this->interrupt_on_short_pkt = interrupt_on_short_pkt;
      this->no_snoop = no_snoop;
      this->chain = chain;
      this->interrupt_on_complete = interrupt_on_complete;
      this->immediate_data = immediate_data;
      this->block_event_interrupt = block_event_interrupt;
    };
  };
  static_assert(sizeof(normal_transfer_trb) == 16, "Normal transfer TRB size wrong.");

  /// @brief A transfer setup stage TRB.
  ///
  struct setup_stage_transfer_trb
  {
    uint16_t request_type : 8; ///< Equivalent to 'bmRequestType'
    uint16_t request : 8; ///< Equivalent to 'bRequest'
    uint16_t value; ///< Equivalent to 'wValue'
    uint16_t index; ///< Equivalent to 'wIndex'
    uint16_t length; ///< Equivalent to 'wLength'
    uint32_t trb_transfer_length : 17; ///< The number of bytes in the transfer.
    uint32_t reserved_1 : 5; ///< Reserved
    uint32_t interrupter_target : 10; ///< The target interrupter.

    uint32_t cycle : 1; ///< Cycle bit
    uint32_t reserved_2 : 4; ///< Reserved
    uint32_t interrupt_on_complete : 1; ///< Interrupt once the transfer is complete.
    uint32_t immediate_data : 1; ///< Specifies immediate data in some part of the TRB
    uint32_t reserved_3 : 3; ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::SETUP_STAGE
    uint32_t transfer_type : 2; ///< 0 = no data stage, 2 = OUT data stage, 3 = IN data stage (2 = reserved)
    uint32_t reserved_4 : 14; ///< Reserved.

    /// @cond
    void populate(uint8_t request_type,
                  uint8_t request,
                  uint16_t value,
                  uint16_t index,
                  uint16_t length,
                  uint16_t interrupter,
                  bool cycle,
                  bool ioc,
                  bool immediate,
                  uint16_t transfer_type)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      this->trb_type = TRB_TYPES::SETUP_STAGE;
      this->trb_transfer_length = 8;

      this->request_type = request_type;
      this->request = request;
      this->value = value;
      this->index = index;
      this->length = length;
      this->interrupter_target = interrupter;

      this->cycle = cycle;
      this->interrupt_on_complete = ioc;
      this->immediate_data = immediate;
      this->transfer_type = transfer_type;
    };
    /// @endcond
  };
  static_assert(sizeof(setup_stage_transfer_trb) == 16, "Setup stage transfer TRB size wrong.");

  /// @brief A transfer data stage TRB.
  ///
  struct data_stage_transfer_trb
  {
    uint64_t data_buffer_ptr_phys; ///< Pointer to buffer in memory (or immediate data)
    uint32_t trb_transfer_length : 17; ///< The number of bytes to transfer.
    uint32_t td_size : 5; ///< The expected number of packets in the transfer.
    uint32_t interrupter_target : 10; ///< The target interrupter.

    int32_t cycle : 1; ///< Cycle bit
    uint32_t evaluate_next_trb : 1; ///< Evaluate next TRB before saving the state of the endpoint.
    uint32_t interrupt_on_short_pkt : 1; ///< Interrupt if the device returns a short packet.
    uint32_t no_snoop : 1; ///< Set the PCI no snoop attribute.
    uint32_t chain : 1; ///< Chain bit
    uint32_t interrupt_on_complete : 1; ///< Interrupt once TRB completes.
    uint32_t immediate_data : 1; ///< If 1, data_buffer_ptr_phys contains immediate data.
    uint32_t reserved_1 : 3; ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::DATA_STAGE.
    uint32_t is_in_dir : 1; ///< If 1, IN direction transfer. If 0, OUT direction transfer.
    uint32_t reserved_2 : 15; ///< Reserved.

    /// @cond
    void populate(uint64_t data_buffer_phys,
                  uint32_t transfer_len,
                  uint16_t td_size,
                  uint16_t interrupter,
                  bool cycle,
                  bool evaluate_next_trb,
                  bool interrupt_on_short_pkt,
                  bool no_snoop,
                  bool chain,
                  bool interrupt_on_complete,
                  bool immediate_data,
                  bool is_inwards)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      this->trb_type = TRB_TYPES::DATA_STAGE;

      this->data_buffer_ptr_phys = data_buffer_phys;
      this->trb_transfer_length = transfer_len;
      this->td_size = td_size;
      this->interrupter_target = interrupter;
      this->cycle = cycle;
      this->evaluate_next_trb = evaluate_next_trb;
      this->interrupt_on_short_pkt = interrupt_on_short_pkt;
      this->no_snoop = no_snoop;
      this->chain = chain;
      this->interrupt_on_complete = interrupt_on_complete;
      this->immediate_data = immediate_data;
      this->is_in_dir = is_inwards;
    };
    /// @endcond
  };
  static_assert(sizeof(data_stage_transfer_trb) == 16, "Data stage transfer TRB size wrong.");

  /// @brief A transfer status stage TRB.
  ///
  struct status_stage_transfer_trb
  {
    uint64_t reserved_1; ///< Reserved.
    uint32_t reserved_2 : 22; ///< Reserved.
    uint32_t interrupter_target : 10; ///< The target interrupter.

    uint32_t cycle : 1; ///< Cycle bit.
    uint32_t evaluate_next_trb : 1; ///< Evaluate next TRB before saving endpoint state.
    uint32_t reserved_3 : 2; ///< Reserved.
    uint32_t chain : 1; ///< Chain bit
    uint32_t interrupt_on_complete : 1; ///< Interrupt once TRB complete.
    uint32_t reserved_4 : 4; ///< Reserved.
    uint32_t trb_type : 6; ///< Set to TRB_TYPES::STATUS_STAGE.
    uint32_t is_in_dir : 1; ///< If 1, is an IN transfer, otherwise an OUT transfer.
    uint32_t reserved_5 : 15; ///< Reserved.

    /// @cond
    void populate(uint16_t interrupter,
                  bool cycle,
                  bool evaluate_next_trb,
                  bool chain,
                  bool interrupt_on_complete,
                  bool is_inwards)
    {
      reserved_1 = 0;
      reserved_2 = 0;
      reserved_3 = 0;
      reserved_4 = 0;
      reserved_5 = 0;
      this->trb_type = TRB_TYPES::STATUS_STAGE;

      this->interrupter_target = interrupter;
      this->cycle = cycle;
      this->evaluate_next_trb = evaluate_next_trb;
      this->chain = chain;
      this->interrupt_on_complete = interrupt_on_complete;
      this->is_in_dir = is_inwards;
    };
    /// @endcond
  };
  static_assert(sizeof(status_stage_transfer_trb) == 16, "Status stage transfer TRB size wrong.");

#pragma pack(pop)

  class device_core;

}; };