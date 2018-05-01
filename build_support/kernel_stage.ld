OUTPUT_FORMAT("binary")
ENTRY(pre_main_32)

/******************************************************************************
 * Throughout this script we realign the position counter, and then calculate *
 * the load address from that. This works around a problem were LD sometimes  *
 * realigns sections, so simply using the size of the preceeding section      *
 * to calculate the load address of the next section leaves you out of sync   *
 * with what LD is doing. Fortunately, it seems like forcing the alignment to *
 * 1kb boundaries stops LD doing this. Maybe a smaller alignment would work.  *
 ******************************************************************************/

SECTIONS
{
  . = 0x0000000000100000;

  /*******************************************************************************************************************
  /* BSP 32-bit entry point from multiboot. Must be first, to ensure the entry point can be found by the multiboot   *
   * loader.                                                                                                         *
   *******************************************************************************************************************/
  .pretext_32 :
  {
      *(.pretext_32)
  }
  . = ALIGN(4096);
  start_loc = .;

  /*******************************************************************************************************************
   * All other code that must be linked against lower memory (e.g. AP entry point, long mode initialization).        *
   *******************************************************************************************************************/
  .pretext : AT ( start_loc )
  {
      *(.pretext)
  }
  . = 0xFFFFFFFF00000000 + .;
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .text : AT ( start_loc )
  {
    *(.text)
  }
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .data : AT ( start_loc )
  {
    *(.data)
  }
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .init_array : AT ( start_loc )
  {
      *(.init_array)
  }
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .bss : AT ( start_loc )
  {
    *(.bss)
    *(COMMON)
  }
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .rodata : AT ( start_loc )
  {
      *(.rodata)
  }
  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  .eh_frame : AT ( start_loc )
  {
      *(.eh_frame)
  }

  . = ALIGN(1024);
  start_loc = . - 0xFFFFFFFF00000000;
  asm_ap_trampoline_addr = .;
  . = 0x1000;
  .pretext_16 : AT ( start_loc )
  {
      *(.pretext_16)
  }
}
