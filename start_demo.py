#!/usr/bin/python3
# Start a demo machine

import argparse
import configparser
import os

sys_root = None

def start_demo(config_file):
  try:
    cfg_file = open(config_file, "r")
    cfg = configparser.ConfigParser()
    cfg.read_file(cfg_file)
    cfg_file.close()

    sys_root = cfg["PATHS"]["sys_image_root"]
  except:
    print("Failed to load configuration - run builder.py first.")
    return

  # Several options here are commented out - they are reminders of options that you may find useful.
  qemu_params = ["qemu-system-x86_64",
                 "-m 256M",
                 "-drive file=fat:rw:fat-type=16:{root},format=raw,media=disk",
                 #"-drive file=fat:rw:fat-type=32:.,format=raw,media=disk,if=none,id=virtio_disk",
                 #"-device virtio-blk,drive=virtio_disk,disable-legacy=on,disable-modern=off",
                 "-no-reboot",
                 "-no-shutdown",
                 "-smp cpus=2",
                 "-cpu Haswell",
                 "-serial stdio", # Could change this to -debugcon stdio to put the monitor on stdio.
                 #"-serial telnet:localhost:4321,server", #,nowait",
                 "-device nec-usb-xhci",
                 "-kernel {root}/kernel64.sys",
                 "-vga std",
                 "-machine pc,accel=kvm:whpx:xen:hax:hvf:tcg",
                 #"-serial vc:80Cx24C"

                 #"-D /tmp/qemu.log",
                 #"-d int,pcall,cpu_reset",
                 #"-d in_asm,nochain,int,cpu_reset,exec",
                 #"-dfilter 0xFFFFFFFF00000000..0xffffffffffffffff",
                 #"--trace events=/tmp/trcevents",
                 #"-device usb-kbd",
                 #"-hda ../project_azalea/kernel_disc_image_nonlive.vdi",

                 # Attempting record/replay - this is incredibly slow!
                 #"-drive file=fat:ro:fat-type=16:{root},format=raw,media=disk,if=none,snapshot,id=img-direct",
                 #"-drive driver=blkreplay,if=none,image=img-direct,id=img-blkreplay",
                 #"-device ide-hd,drive=img-blkreplay",
                 #"-icount shift=7,rr=record,rrfile=replay.bin,rrsnapshot=snapshot.bin",
                ]

  qemu_cmd = " ".join(qemu_params)
  qemu_cmd = qemu_cmd.format(**{'root' : sys_root})
  os.system(qemu_cmd)

if __name__ == "__main__":
  argp = argparse.ArgumentParser(description = "Project Azalea Builder helper")
  argp.add_argument("--config_file", type = str, default = "build_config.ini", help = "Config file location")
  args = argp.parse_args()

  start_demo(args.config_file)