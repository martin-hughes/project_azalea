#pragma once

#include "stdint.h"

namespace usb { namespace xhci {

#pragma pack(push, 1)
  /// @brief xHCI capabilities registers.
  ///
  struct caps_regs
  {
    /// The length of the capability registers. May be longer than this structure, since some space is reserved after
    /// this structure before the operational registers.
    uint8_t caps_length;

    uint8_t reserved; ///< Reserved.

    uint16_t hci_version; ///< Interface version number.

    union
    {
      uint32_t struct_params_1_raw; ///< Structural parameters #1, HCSPARAMS1
      struct
      {
        uint32_t max_device_slots : 8;
        uint32_t max_interrupters : 11;
        uint32_t reserved : 5;
        uint32_t max_ports : 8;
      } struct_params_1;
    };
    union
    {
      uint32_t struct_params_2_raw; ///< Structural parameters #2, HCSPARAMS2
      struct
      {
        uint32_t isoch_sched_threshold : 4;
        uint32_t erst_max : 4;
        uint32_t extended_tbc_enable : 1;
        uint32_t reserved : 12;
        uint32_t max_scratchpad_bufs_hi : 5;
        uint32_t scratchpad_restore : 1;
        uint32_t max_scratchpad_bufs_lo : 5;
      } struct_params_2;
    };
    uint32_t struct_params_3; ///< Structural parameters #3, HCSPARAMS3

    uint32_t capability_params_1; ///< Capability parameters #1

    uint32_t doorbell_offset; ///< Offset to the doorbell array.

    uint32_t runtime_regs_offset; ///< Offset to the runtime registers.

    uint32_t capability_params_2; ///< Capability parameters #2
  };
  static_assert(sizeof(caps_regs) == 32, "xHCI capabilities registers size wrong.");

  struct oper_regs
  {
    // Mnemonics taken from the Intel xHCI specification are shown as the Doxygen comments.
    union
    {
      uint32_t usb_command_raw; ///< USBCMD register
      struct
      {
        uint32_t run_stop : 1; ///< Run/stop flag
        uint32_t hc_reset : 1; ///< Host Controller reset.
        uint32_t interrupter_enable : 1; ///< INTE
        uint32_t host_sys_err_enable : 1; ///< HSEE
        uint32_t reserved_1 : 3;
        uint32_t lhc_reset : 1; ///< Lightweight host controller reset.
        uint32_t controller_save_state : 1; ///<CSS
        uint32_t controller_restore_state : 1; ///< CRS
        uint32_t enable_wrap_event : 1; ///< EWE
        uint32_t enable_u3_mfindex_stop : 1; ///< EU3S
        uint32_t reserved_2 : 1;
        uint32_t cem_enable : 1; ///<CME
        uint32_t reserved_3 : 18;
      } usb_command;
    };
    volatile union
    {
      uint32_t usb_status_raw; ///< USBSTATUS
      struct
      {
        uint32_t host_ctrlr_halted : 1; ///< Host controller halted, HCH.
        uint32_t reserved_1 : 1;
        uint32_t host_system_err : 1; ///< Host system error, HSE.
        uint32_t event_interrupt : 1; ///< Event interrupt, EINT.
        uint32_t port_change_detect : 1; ///< Port change detected, PCD.
        uint32_t reserved_2 : 3;
        uint32_t save_state_status : 1; ///< Save state status, SSS.
        uint32_t restore_state_status : 1; ///< Restore state status, RSS.
        uint32_t save_restore_err : 1; ///< Save/Restore error, SRE.
        uint32_t controller_not_ready : 1; ///< Controller not ready, CNR.
        uint32_t host_controller_error : 1; ///< Host controller error, HCE.
        uint32_t reserved_3 : 19;
      } usb_status;
    };
    uint32_t page_size; ///< PAGESIZE
    uint64_t reserved_1;
    uint32_t dev_notn_cntrl; /// Device notification control, DNCTRL
    uint64_t cmd_ring_cntrl; /// Command ring control, CRCR
    uint64_t reserved_2;
    uint64_t reserved_3;
    uint64_t dev_cxt_base_addr_ptr; /// Device Context Base Address Array Pointer, DCBAAP
    union
    {
      uint32_t configure_raw; ///< CONFIG
      struct
      {
        uint32_t max_device_slots_enabled : 8; ///< Max device slots enabled, MaxSlotsEn
        uint32_t u3_entry_enable : 1; ///< U3 entry enabled, U3E.
        uint32_t config_info_enable : 1; ///< Configuration Information Enable, CIE
        uint32_t reserved : 22;
      } configure;
    };
  };
  static_assert(sizeof(oper_regs) == 60, "xHCI operational registers size wrong.");

  // At present this is just a fixed size buffer, I'll fill in the details later on.
  struct device_context
  {
    char context[0x400];
  };
  static_assert(sizeof(device_context) == 0x400, "Device context size wrong");

  struct interrupter_regs
  {
    union
    {
      uint32_t raw;
      struct
      {
        uint32_t pending : 1;
        uint32_t enable : 1;
        uint32_t reserved : 30;
      };
    } interrupt_management;
    uint32_t interrupt_moderation;
    uint16_t table_size;
    uint16_t reserved_1;
    uint32_t reserved_2;
    uint64_t erst_base_addr_phys;
    uint64_t erst_dequeue_ptr_phys;
  };
  static_assert(sizeof(interrupter_regs) == 32, "Sizeof event ring registers wrong.");

#pragma pack(pop)

}; };