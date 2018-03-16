// Provides an interface for controlling I/O APIC controllers.

//#define ENABLE_TRACING

#include "klib/klib.h"
#include "ioapic-x64.h"
#include "acpi/acpi_if.h"

static unsigned long ioapic_count = 0;

const unsigned char SUBTABLE_IOAPIC_TYPE = 1;

struct ioapic_data
{
  // Translated values
  unsigned int *reg_select;
  unsigned int *data_window;

  // Raw values
  unsigned char apic_id;
  unsigned int apic_addr;
  unsigned int gs_interrupt_base;
};

static klib_list<ioapic_data *> ioapic_list;

void proc_x64_ioapic_add_ioapic(acpi_madt_io_apic *table);

void proc_x64_ioapic_set_redir_tab(ioapic_data *ioapic, unsigned char num_in, unsigned char vector_out, unsigned char apic_id);

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
  while(((unsigned long)subtable - (unsigned long)madt_table) < madt_table->Header.Length)
  {
    KL_TRC_DATA("Found a new table of type", (unsigned long)subtable->Type);

    if (subtable->Type == SUBTABLE_IOAPIC_TYPE)
    {
      proc_x64_ioapic_add_ioapic((acpi_madt_io_apic *)subtable);
    }

    subtable = acpi_advance_subtable_ptr(subtable);
  }

  KL_TRC_EXIT;
}

unsigned long proc_x64_ioapic_get_count()
{
  KL_TRC_ENTRY;
  KL_TRC_DATA("Number of I/O APICs known", ioapic_count);
  KL_TRC_EXIT;

  return ioapic_count;
}

void proc_x64_ioapic_add_ioapic(acpi_madt_io_apic *table)
{
  KL_TRC_ENTRY;

  unsigned long ioapic_phys_base;
  unsigned long ioapic_offset;
  unsigned long virtual_addr;

  klib_list_item<ioapic_data *> *new_item = new klib_list_item<ioapic_data *>;
  ioapic_data *data = new ioapic_data;

  klib_list_item_initialize(new_item);
  new_item->item = data;

  data->apic_id = table->Id;
  data->apic_addr = table->Address;
  data->gs_interrupt_base = table->GlobalIrqBase;

  KL_TRC_DATA("APIC ID", (unsigned long)table->Id);
  KL_TRC_DATA("APIC address", table->Address);
  KL_TRC_DATA("GSI Base", table->GlobalIrqBase);

  klib_list_add_tail(&ioapic_list, new_item);

  // Map this IOAPICs registers.
  ioapic_offset = table->Address % MEM_PAGE_SIZE;
  ioapic_phys_base = table->Address - ioapic_offset;
  virtual_addr = (unsigned long)mem_allocate_virtual_range(1);
  mem_map_range((void *)ioapic_phys_base, (void *)virtual_addr, 1, nullptr, MEM_UNCACHEABLE);

  data->reg_select = (unsigned int *)(virtual_addr + ioapic_offset);
  data->data_window = (unsigned int *)(virtual_addr + ioapic_offset + 16);

  ioapic_count++;

  KL_TRC_EXIT;
}

// Remap an IO APICs inputs to interrupts starting from the vector number at base_int.
void proc_x64_ioapic_remap_interrupts(unsigned int ioapic_num, unsigned char base_int, unsigned char apic_id)
{
  KL_TRC_ENTRY;

  int temp_vect;

  KL_TRC_DATA("IO APIC number", ioapic_num);
  KL_TRC_DATA("Base interrupt", base_int);
  KL_TRC_DATA("APIC ID to route to", apic_id);

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
void proc_x64_ioapic_set_redir_tab(ioapic_data *ioapic, unsigned char num_in, unsigned char vector_out, unsigned char apic_id)
{
  KL_TRC_ENTRY;

  const unsigned int INP_ZERO_REG = 0x10;

  unsigned int vector_data_low;
  unsigned int vector_data_high;

  *ioapic->reg_select = INP_ZERO_REG + 2 * num_in;
  vector_data_low = *ioapic->data_window;

  *ioapic->reg_select = INP_ZERO_REG + (2 * num_in) + 1;
  vector_data_high = *ioapic->data_window;

  vector_data_high = vector_data_high & 0x00FFFFFF;
  vector_data_high = vector_data_high | (((unsigned int)apic_id) << 24);

  // This rather odd mask preserves all known RO fields.
  vector_data_low = vector_data_low & 0xFFFE5000;
  vector_data_low |= vector_out;

  *ioapic->reg_select = INP_ZERO_REG + 2 * num_in;
  *ioapic->data_window = vector_data_low;

  *ioapic->reg_select = INP_ZERO_REG + (2 * num_in) + 1;
  *ioapic->data_window = vector_data_high;

  KL_TRC_EXIT;
}
