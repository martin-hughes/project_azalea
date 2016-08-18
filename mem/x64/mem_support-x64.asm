[BITS 64]
SEGMENT .text

GLOBAL mem_invalidate_page_table
mem_invalidate_page_table:
  ; TODO: Consider how to invalidate page tables on another processor (MT)
  invlpg [rdi]
  ret
