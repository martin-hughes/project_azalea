# Calculate the number of chunks in a slab and the offset of the first chunk,
# given a list of chunk sizes.
#
# Used to update values in the memory allocator.

CHUNK_SIZES = (8, 64, 256, 1024, 262144)
NUM_CHUNKS = [ ]
FIRST_OFFSET = [ ]
WASTED_BYTES = [ ]

HEADER_SIZE = 40
BITMAP_GROW_BY = 8 # Bitmap is formed of 8-byte longs
SLAB_SIZE = 2097152 # 2MB

print("Chunk size | Num chunks | First offset | Wasted bytes")
print("-----------|------------|--------------|-------------")
for chunk_size in CHUNK_SIZES:
  free_space = SLAB_SIZE - HEADER_SIZE
  basic_num_chunks = int(free_space / chunk_size)
  bytes_of_bitmap = int(basic_num_chunks / 8) + 1
  bitmap_lumps = int(bytes_of_bitmap / 8) + 1
  true_bitmap_bytes = bitmap_lumps * 8
  
  min_first_offset = HEADER_SIZE + true_bitmap_bytes
  chunks_unavailable = int(min_first_offset / chunk_size) + 1
  true_first_offset = chunks_unavailable * chunk_size
  
  FIRST_OFFSET.append(true_first_offset)
  
  true_num_chunks = int((SLAB_SIZE - true_first_offset) / chunk_size)
  
  NUM_CHUNKS.append(true_num_chunks)
  
  bytes_wasted = SLAB_SIZE - (true_num_chunks * chunk_size) - HEADER_SIZE - true_bitmap_bytes
  
  WASTED_BYTES.append(bytes_wasted)
  
  print(" {0: <9} | {1: <10} | {2: <12} | {3: <}".format(chunk_size, true_num_chunks, true_first_offset, bytes_wasted))