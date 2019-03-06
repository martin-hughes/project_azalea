/// @file
/// @brief Declare structures useful for ATA devices.

#pragma once

#include <stdint.h>

namespace ata
{

#pragma pack (push, 1)

/// @brief Contains the results from an IDENTIFY Command
///
/// The contents of this structure are defined in the ATA Protocol specification, Table 22, so are not documented
/// further.
struct identify_cmd_output
{
  /// @cond
  uint16_t gen_config_bits;
  uint16_t obsolete_1;
  uint16_t specific_cfg;
  uint16_t obsolete_2[4];
  uint16_t reserved[2];
  uint16_t obsolete_3;
  char serial_number[20];
  uint16_t obsolete_4[3];
  char firmware_revision[8];
  char model_number[40];
  uint16_t rw_mult_sector_max;
  uint16_t trusted_comp_feature_opts;
  uint16_t capabilities_1;
  uint16_t capabilities_2;
  uint16_t obsolete_5[2];
  union
  {
    uint16_t raw;
    struct
    {
      uint8_t obsolete : 1;
      uint8_t words_64_to_70_valid : 1;
      uint8_t word_88_valid : 1;
      uint8_t reserved : 5;
      uint8_t freefall_sensitivity;
    };
  } freefall_and_validity;
  uint16_t obsolete_6[5];
  uint16_t rw_mult_cur_sector_count;
  uint32_t num_logical_sectors_28;
  uint16_t obsolete_7;
  union
  {
    uint16_t raw;
    struct
    {
      uint16_t modes_supported : 3;
      uint16_t reserved_1 : 5;
      uint16_t modes_enabled : 3;
      uint16_t reserved_2 : 5;
    };
  } multiword_dma_mode;
  uint16_t pio_modes_supported;
  uint16_t min_mdma_transfer_cycle_time;
  uint16_t recmd_mdma_transfer_cycle_time;
  uint16_t min_pio_cycle_time_no_fc;
  uint16_t min_pio_cycle_time_iordy;
  uint16_t reserved_2[2];
  uint16_t reserved_id_packet_cmd[4];
  uint16_t queue_depth;
  uint16_t sata_capabilities;
  uint16_t reserved_sata;
  uint16_t sata_features_supported;
  uint16_t sata_features_enabled;
  uint16_t major_revision;
  uint16_t minor_revision;
  uint16_t cmd_set_supported_1;
  union
  {
    uint16_t cmd_set_supported_2_raw;
    struct
    {
      uint16_t download_ucode_cmd : 1;
      uint16_t rw_dma_queued_cmd : 1;
      uint16_t cfa_features : 1;
      uint16_t apm_features : 1;
      uint16_t obsolete_feature : 1;
      uint16_t power_up_in_standby : 1;
      uint16_t set_features_subcmd_reqd : 1;
      uint16_t reserved_incits_2 : 1;
      uint16_t set_max_security_ext : 1;
      uint16_t auto_acoustic_man : 1;
      uint16_t lba_48 : 1;
      uint16_t device_config_overlay : 1;
      uint16_t mandatory_flush_cache_cmd : 1;
      uint16_t flush_cache_ext_cmd : 1;
      uint16_t set_to_one : 1;
      uint16_t set_to_zero : 1;
    };
  };
  uint16_t cmd_set_supported_3;
  uint16_t cmd_set_supported_4;
  uint16_t cmd_set_supported_5;
  uint16_t cmd_set_supported_6;
  union
  {
    uint16_t raw;
    struct
    {
      uint16_t modes_supported : 7;
      uint16_t reserved_1 : 1;
      uint16_t modes_enabled : 7;
      uint16_t reserved_2 : 1;
    };
  } udma_modes;
  uint16_t secure_erase_unit_time;
  uint16_t enhanced_sec_erase_time;
  uint16_t power_man_value;
  uint16_t master_password_id;
  uint16_t hardware_reset_result;
  uint16_t acoustic_mgmt;
  uint16_t stream_min_rqst_size;
  uint16_t stream_transfer_time_dma;
  uint16_t stream_latency;
  uint16_t stream_perf_gran[2];
  uint64_t num_logical_sectors_48;
  uint16_t stream_transfer_time_pio;
  uint16_t reserved_3;
  uint16_t phys_log_sector_size;
  uint16_t inter_seek_delay;
  uint16_t ieee_oui_1;
  uint16_t ieee_oui_2;
  uint16_t unique_id_2;
  uint16_t unique_id_1;
  uint16_t reserved_name_extension[4];
  uint16_t reserved_incits;
  uint32_t words_per_sector;
  uint16_t supported_settings;
  uint16_t cmd_set_supported_7;
  uint16_t reserved_extended_settings[6];
  uint16_t obsolete_8;
  uint16_t security_status;
  uint16_t vendor_specific[31];
  uint16_t cfa_power_mode_1;
  uint16_t reserved_cflash[15];
  char media_serial_number[60];
  uint16_t sct_command_transport;
  uint16_t reserved_ce_ata_1[2];
  uint16_t logical_block_alignment;
  uint16_t wrv_sector_count_m3[2];
  uint16_t wrv_sector_count_m2[2];
  uint16_t nv_cache_caps;
  uint32_t nv_cache_blocks;
  uint16_t media_rotation_rate;
  uint16_t reserved_4;
  uint16_t nv_cache_options;
  uint16_t wrv_features_supported;
  uint16_t reserved_5;
  uint16_t transport_major_revision;
  uint16_t transport_minor_revision;
  uint16_t reserved_ce_ata_2[10];
  uint16_t min_ucode_units;
  uint16_t max_ucode_units;
  uint16_t reserved_6[19];
  uint16_t integrity_word;
  /// @endcond
};
static_assert(sizeof(identify_cmd_output) == 512, "Sizeof identify_cmd_output wrong.");

/// @brief Standard ATA status byte.
///
struct status_byte
{
  /// @cond
  uint8_t error_flag :1;
  uint8_t reserved :2;
  uint8_t data_ready_flag :1;
  uint8_t overlapped_service_flag :1;
  uint8_t drive_fault_flag :1;
  uint8_t drive_ready_flag :1;
  uint8_t busy_flag :1;
  /// @endcond
};
static_assert(sizeof(status_byte) == 1, "Incorrect packing of status_byte");


#pragma pack (pop)

}; // Namespace ata
