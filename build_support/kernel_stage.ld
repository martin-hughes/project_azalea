OUTPUT_FORMAT("binary")
ENTRY(pre_main)

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
    .pretext : 
    {
        *(.pretext)
    }
    . = 0xFFFFFFFF00100000 + SIZEOF(.pretext);
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
}
