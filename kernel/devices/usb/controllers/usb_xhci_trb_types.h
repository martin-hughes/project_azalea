#pragma once

#include <stdint.h>

namespace usb { namespace xhci
{
  namespace TRB_TYPES
  {
    const uint8_t RESERVED = 0;
    const uint8_t NORMAL = 1;
    const uint8_t SETUP_STAGE = 2;
    const uint8_t DATA_STAGE = 3;
    const uint8_t STATUS_STAGE = 4;
    const uint8_t ISOCH = 5;
    const uint8_t LINK = 6;
    const uint8_t EVENT_DATA = 7;
    const uint8_t NO_OP = 8;
    const uint8_t ENABLE_SLOT_CMD = 9;
    const uint8_t DISABLE_SLOT_CMD = 10;
    const uint8_t ADDRESS_DEVICE_CMD = 11;
    const uint8_t CONFIG_ENDPOINT_CMD = 12;
    const uint8_t EVAL_CONTEXT_CMD = 13;
    const uint8_t RESET_ENDPOINT_CMD = 14;
    const uint8_t STOP_ENDPOINT_CMD = 15;
    const uint8_t SET_TR_DEQUEUE_PTR_CMD = 16;
    const uint8_t RESET_DEVICE_CMD = 17;
    const uint8_t FORCE_EVENT_CMD = 18;
    const uint8_t NEGOTIATE_BANDWIDTH_CMD = 19;
    const uint8_t SET_LATENCY_TOL_CMD = 20;
    const uint8_t GET_PORT_BANDWIDTH_CMD = 21;
    const uint8_t FORCE_HEADER_CMD = 22;
    const uint8_t NO_OP_CMD = 23;
    const uint8_t TRANSFER_EVENT = 32;
    const uint8_t COMMAND_COMPLETE_EVENT = 33;
    const uint8_t PORT_STS_CHANGE_EVENT = 34;
    const uint8_t BANDWIDTH_REQUEST_EVENT = 35;
    const uint8_t DOORBELL_EVENT = 36;
    const uint8_t HOST_CONTROLLER_EVENT = 37;
    const uint8_t DEVICE_NOTFN_EVENT = 38;
    const uint8_t MFINDEX_WRAP_EVENT = 39;
  };

#pragma pack(push, 1)
  /// @brief The most basic TRB - an empty one.
  ///
  struct basic_trb
  {
    uint64_t reserved_1;
    uint64_t reserved_2;

    /// @brief Fills the TRBs with zeroes.
    void populate()
    {
      reserved_1 = 0;
      reserved_2 = 0;
    };
  };
  static_assert(sizeof(basic_trb) == 16, "Basic TRB size wrong.");

  struct template_trb
  {
    uint64_t data;
    uint32_t status;

    uint32_t cycle : 1;
    uint32_t evaluate_next_trb : 1;
    uint32_t reserved_1 : 8;
    uint32_t trb_type : 6;
    uint32_t reserved_4 : 16;
  };
  static_assert(sizeof(template_trb) == 16, "Template TRB size wrong.");

  /// @brief Link TRBs link the ring to the next TRB.
  ///
  /// They are also used at the end of the ring to link back to the first TRB.
  struct link_trb
  {
    uint64_t trb_addr;
    uint32_t reserved_1 : 22;
    uint32_t interrupter : 10;
    uint32_t cycle : 1;
    uint32_t toggle_cycle : 1;
    uint32_t reserved_2 : 2;
    uint32_t chain : 1;
    uint32_t interrupt_on_complete : 1;
    uint32_t reserved_3 : 4;
    uint32_t trb_type : 6;
    uint32_t reserved_4 : 16;

    /// @brief Create a link TRB.
    ///
    /// @param next_trb_phys_addr
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

  struct no_op_cmd_trb
  {
    uint64_t reserved_1;
    uint32_t reserved_2;
    uint32_t cycle : 1;
    uint32_t reserved_3 : 9;
    uint32_t trb_type : 6;
    uint32_t reserved_4 : 16;

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
#pragma pack(pop)
}; };