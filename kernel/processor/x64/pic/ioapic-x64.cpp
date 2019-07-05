// Provides an interface for controlling I/O APIC controllers.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "ioapic-x64.h"
#include "acpi/acpi_if.h"

static uint64_t ioapic_count = 0;

const uint8_t SUBTABLE_IOAPIC_TYPE = 1;

/// @brief Stores data about one IO APIC attached to the system.
///
struct ioapic_data
{
  // Translated values
  uint32_t *reg_select; ///< The virtual address to write register-select values in to.
  uint32_t *data_window; ///< Having written reg_select, the relevant data is read/written here.

  // Raw values
  uint8_t apic_id; ///< The system's ID number associated with this APIC.
  uint32_t apic_addr; ///< The physical address of this APIC.
  uint32_t gs_interrupt_base; ///< The BaseIRQ number for this IO APIC.
};

static klib_list<ioapic_data *> ioapic_list;

void proc_x64_ioapic_add_ioapic(acpi_madt_io_apic *table);

void proc_x64_ioapic_set_redir_tab(ioapic_data *ioapic, uint8_t num_in, uint8_t vector_out, uint8_t apic_id);

void proc_x64_ioapic_load_data()
{
  KL_TRC_ENTRY;

  ACPI_STATUS retval;
  char table_name[] = "APIC";
  acpi_table_madt *madt_table;
  acpi_subtable_header *subtable;

  klib_list_initialize(&ioapic_list);

  retval = AcpiGetTable((ACPI_STRING)table_name, 0, (ACPI_TABLE_HEADER **)&madt_table);
  ASSERT(retval == AE_OK);
  ASSERT(madt_table->Header.Length > sizeof(acpi_table_madt));

  subtable = acpi_init_subtable_ptr((void *)madt_table, sizeof(acpi_table_madt));
  while(((uint64_t)subtable - (uint64_t)madt_table) < madt_table->Header.Length)
  {
    KL_TRC_TRACE(TRC_LVL::EXTRA, "Found a new table of type", subtable->Type, "\n");

    if (subtable->Type == SUBTABLE_IOAPIC_TYPE)
    {
      proc_x64_ioapic_add_ioapic((acpi_madt_io_apic *)subtable);
    }

    subtable = acpi_advance_subtable_ptr(subtable);
  }

  KL_TRC_EXIT;
}

uint64_t proc_x64_ioapic_get_count()
{
  KL_TRC_ENTRY;
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Number of I/O APICs known", ioapic_count, "\n");
  KL_TRC_EXIT;

  return ioapic_count;
}

void proc_x64_ioapic_add_ioapic(acpi_madt_io_apic *table)
{
  KL_TRC_ENTRY;

  uint64_t ioapic_phys_base;
  uint64_t ioapic_offset;
  uint64_t virtual_addr;

  klib_list_item<ioapic_data *> *new_item = new klib_list_item<ioapic_data *>;
  ioapic_data *data = new ioapic_data;

  klib_list_item_initialize(new_item);
  new_item->item = data;

  data->apic_id = table->Id;
  data->apic_addr = table->Address;
  data->gs_interrupt_base = table->GlobalIrqBase;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "APIC ID", table->Id, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "APIC address", table->Address, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "GSI Base", table->GlobalIrqBase, "\n");

  klib_list_add_tail(&ioapic_list, new_item);

  // Map this IOAPICs registers.
  ioapic_offset = table->Address % MEM_PAGE_SIZE;
  ioapic_phys_base = table->Address - ioapic_offset;
  virtual_addr = (uint64_t)mem_allocate_virtual_range(1);
  mem_map_range((void *)ioapic_phys_base, (void *)virtual_addr, 1, nullptr, MEM_UNCACHEABLE);

  data->reg_select = (uint32_t *)(virtual_addr + ioapic_offset);
  data->data_window = (uint32_t *)(virtual_addr + ioapic_offset + 16);

  ioapic_count++;

  KL_TRC_EXIT;
}

// Remap an IO APICs inputs to interrupts starting from the vector number at base_int.
void proc_x64_ioapic_remap_interrupts(uint32_t ioapic_num, uint8_t base_int, uint8_t apic_id)
{
  KL_TRC_ENTRY;

  int temp_vect;

  KL_TRC_TRACE(TRC_LVL::EXTRA, "IO APIC number", ioapic_num, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "Base interrupt", base_int, "\n");
  KL_TRC_TRACE(TRC_LVL::EXTRA, "APIC ID to route to", apic_id, "\n");

  ASSERT(ioapic_num == 0);

  klib_list_item<ioapic_data *> *first_item;
  ioapic_data *ioapic;

  first_item = ioapic_list.head;
  ASSERT(first_item != nullptr);
  ASSERT(first_item->item != nullptr);

  ioapic = first_item->item;

  // Remap all the IRQs supported by this IOAPIC. Except for IRQ2 - make that into IRQ0. This is a bit of an ugly hack,
  // but it comes about because the HPET signals IRQ0 on a legacy PIC, but input 2 on a IOAPIC. This hack then stops
  // the rest of the code having to care about whether PIC or APIC mode is being used.
  for (int i = 0; i < 24; i++)
  {
    temp_vect = (i == 2) ?  base_int : i + base_int;
    proc_x64_ioapic_set_redir_tab(ioapic, i, temp_vect, apic_id);
  }

  KL_TRC_EXIT;
}

// Remap an IO APIC's single input to a specified vector at the CPU
void proc_x64_ioapic_set_redir_tab(ioapic_data *ioapic, uint8_t num_in, uint8_t vector_out, uint8_t apic_id)
{
  KL_TRC_ENTRY;

  const uint32_t INP_ZERO_REG = 0x10;

  uint32_t vector_data_low;
  uint32_t vector_data_high;

  *ioapic->reg_select = INP_ZERO_REG + 2 * num_in;
  vector_data_low = *ioapic->data_window;

  *ioapic->reg_select = INP_ZERO_REG + (2 * num_in) + 1;
  vector_data_high = *ioapic->data_window;

  vector_data_high = vector_data_high & 0x00FFFFFF;
  vector_data_high = vector_data_high | (((uint32_t)apic_id) << 24);

  // This rather odd mask preserves all known RO fields.
  vector_data_low = vector_data_low & 0xFFFE5000;
  vector_data_low |= vector_out;

  *ioapic->reg_select = INP_ZERO_REG + 2 * num_in;
  *ioapic->data_window = vector_data_low;

  *ioapic->reg_select = INP_ZERO_REG + (2 * num_in) + 1;
  *ioapic->data_window = vector_data_high;

  KL_TRC_EXIT;
}
