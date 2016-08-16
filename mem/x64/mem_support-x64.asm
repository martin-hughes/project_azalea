[BITS 64]
SEGMENT .text

GLOBAL mem_invalidate_page_table
mem_invalidate_page_table:
  invlpg [rdi]
  ret
