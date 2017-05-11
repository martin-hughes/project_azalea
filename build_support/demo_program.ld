OUTPUT_FORMAT("binary")
ENTRY(pre_main)

SECTIONS
{
  . = 0x0000000000200000;
  .pretext : 
  {
      *(.pretext)
  }
  .text : 
  {
    *(.text)
  }
  .data :
  {
    *(.data)
  }
    
  .init_array :
  {
      *(.init_array)
  }
  .bss : 
  {
    *(.bss)
    *(COMMON)
  }
  .rodata : 
  {
      *(.rodata)
  }
    
  .eh_frame : 
  {
      *(.eh_frame)
  }
}


