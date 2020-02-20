import mapfile
import sys

if __name__ == "__main__":
  f = None

  try:
    f = open(sys.argv[1])
  except (FileNotFoundError, IndexError):
    try:
      f = open("..\\output\\kernel_map.map")
    except:
      print("Usage: stack_trace.py map_file.map [trace_file]")
      exit()

  x = mapfile.mapfile(f)

  f = None
  try:
    f = open(sys.argv[2])
  except (FileNotFoundError, IndexError):
    pass

  while 1:
    if f is None:
      text = input("Symbol address (hex), blank to exit: ")
    else:
      text = f.readline().strip()

    if text[0:4] == "RIP:":
      text = text.split(" ")[1].strip(" ,")

    if text == "":
      exit()

    try:
      addr = int(text, base = 16)
    except ValueError:
      print("... as hex.")
      continue

    symbol = x.find_symbol_by_addr(addr)
    print("0x{addr:x}: {name} + 0x{offset:x}".format(addr = addr, name = symbol.name, offset = addr - symbol.start_addr))
