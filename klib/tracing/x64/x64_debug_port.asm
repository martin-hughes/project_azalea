[BITS 64]
SEGMENT .text

GLOBAL asm_trc_dbg_port_output_string
asm_trc_dbg_port_output_string:
	mov al, [rdi]
	cmp al, 0
	je asm_trc_dbg_port_output_string_end

    out 0xe9, al
    inc rdi
    jmp asm_trc_dbg_port_output_string

asm_trc_dbg_port_output_string_end:
	ret
