
def main(trace_file, map_file, output_file):
  (map_dict, map_offsets) = read_map_file(map_file)
  
  process_trace_file(map_dict, map_offsets, trace_file, output_file)

def read_map_file(map_file):
  map_file_contents = map_file.readlines()
  
  # The map file has all kinds of interesting contents, but the lines we're
  # interested in are exactly of the form:
  #
  # <16 x space><offset - 18 chars><16 x space><symbol name>
  
  map_dict = { }
  
  for line in map_file_contents:
    results = interpret_map_line(line)
    if results is not None:
      map_dict[results[0]] = results[1]
      
  keys = map_dict.keys()
  keys.sort()
  
  return (map_dict, keys)

def process_trace_file(map_dict, map_offsets, trace_file, output_file):
  trace_lines = trace_file.readlines()
  
  for line in trace_lines:
    results = split_trace_line(line)
    if results is not None:
      (offset, instruction) = results
      true_offset = offset
      if offset not in map_dict:
        for temp_offset in map_offsets:
          if temp_offset < offset:
            true_offset = temp_offset
        
        try:
          map_dict[offset] = map_dict[true_offset]
        except:
          pass
        
      fn = "(unknown)"
      try:
        fn= map_dict[offset]
      except:
        pass
        
      output_string = "{0: <16s} ({1: <16x}) {2}\n".format(fn, offset, instruction)
      output_file.write(output_string)
    
    
def split_trace_line(line):
  # The format of a valid line of trace is
  # <offset> <instruction>
  parts = line.split(" ", 1)
  if parts[0] == '':
    return None
  
  if parts[0][-1] != ":":
    return None
  
  offset = 0
  try:
    offset = int(parts[0][0:-1], 16)
  except:
    return None
  
  return (offset, parts[1].strip())

def interpret_map_line(map_file_line):
  symbol_addr = 0
  try:
    symbol_addr = int(map_file_line[16:34], 16)
  except:
    return None
    
  if ((map_file_line[0:16] != (" " * 16)) or 
      (map_file_line[34:50] != (" " * 16)) or
      (map_file_line[50] == ".")):
    return None
  
  if "=" in map_file_line:
    return None

  fn_decl = map_file_line[50:].strip()
  fn_decl = fn_decl.split("(", 1)[0]
  
  results = (symbol_addr, fn_decl)
  return results

if __name__ == "__main__":
  trace_file = open("/tmp/qemu.log")
  map_file = open("/tmp/kernel_map.map")
  output_file = open("/tmp/trace.txt", "w")
  main(trace_file, map_file, output_file)