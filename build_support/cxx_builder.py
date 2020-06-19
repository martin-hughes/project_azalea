#!/usr/bin/python3.8
# Main script for azalea_libcxx_builder
#
# This scripts builds kernel and user-mode versions of libcxx that are customised for use with Project Azalea. It also
# builds libunwind for use within the kernel.
#
# Requires python3.8 or newer.

from contextlib import contextmanager
import os
import configparser
import argparse
import shutil

def main(config):
  os.makedirs("output/kernel_libcxx", exist_ok = True)
  os.makedirs("output/kernel_libunwind", exist_ok = True)
  os.makedirs("output/kernel_libcxxabi", exist_ok = True)
  os.makedirs("output/user_libcxx", exist_ok=True)
  os.makedirs("output/user_libunwind", exist_ok=True)
  os.makedirs("output/user_libcxxabi", exist_ok=True)
  os.makedirs("output/user_compiler-rt", exist_ok=True)

  llvm_path = config["PATHS"]["llvm_base"]
  libcxx_path = os.path.join(llvm_path, "libcxx")
  libcxxabi_path = os.path.join(llvm_path, "libcxxabi")
  libunwind_path = os.path.join(llvm_path, "libunwind")
  compiler_rt_path = os.path.join(llvm_path, "compiler-rt")

  cxx_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx-kernel"))
  unwind_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind-kernel"))
  libcxxabi_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxxabi-kernel"))

  cxx_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx"))
  unwind_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind"))
  libcxxabi_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxxabi"))
  compiler_rt_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "compiler-rt"))

  # Main continues after these lists.
  core_c_flags = [
    "-fno-threadsafe-statics",
    "-nostdinc",
    "-mcmodel=large",
    "-funwind-tables",
    "-isystem %s" % os.path.abspath(os.path.join(libcxx_path, "src")),
    "-isystem %s" % os.path.abspath(os.path.join(libcxx_path, "include")),
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libc", "include")),
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "kernel", "include")),
  ]

  core_cxx_flags = core_c_flags + [
    "-nostdinc++",
    "-std=c++17",
  ]

  kernel_mode_flags = [
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["kernel_base"], "kernel", "include")),
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind-kernel", "include")),
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx-kernel", "include", "c++", "v1")),
    "-fno-use-cxa-atexit",
    "-D _LIBUNWIND_IS_BAREMETAL",
    '-D __AZ_KERNEL__',
  ]

  user_mode_flags = [
    "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind", "include")),
  ]

  cmake_libcxxabi_kernel_cmd = [
    "cmake",
    os.path.join("..", "..", libcxxabi_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % libcxxabi_kernel_install_path,
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLIBCXXABI_ENABLE_ASSERTIONS=OFF",
    "-DCMAKE_BUILD_TYPE=RELEASE",
    "-DLIBCXXABI_HAS_EXTERNAL_THREAD_API=ON",
    "-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF",
    "-DLIBCXXABI_ENABLE_SHARED=OFF",
    "-DLIBCXXABI_ENABLE_STATIC=ON",
    "-DLIBCXXABI_BAREMETAL=ON",
    "-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON",
    "-DLIBCXXABI_ENABLE_PIC=OFF",
    "-DLIBCXXABI_LIBCXX_INCLUDE_DIRS=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
    "-DLIBCXXABI_LIBCXX_INCLUDES=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
  ]

  cmake_libunwind_kernel_cmd = [
    "cmake",
    os.path.join("..", "..", libunwind_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % unwind_kernel_install_path,
    "-DLIBUNWIND_ENABLE_SHARED=OFF",
    "-DLIBUNWIND_ENABLE_STATIC=ON",
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLLVM_ENABLE_LIBCXX=ON",
    "-DLIBUNWIND_ENABLE_ASSERTIONS=OFF",
    "-DCMAKE_BUILD_TYPE=RELEASE",
    "-DLIBUNWIND_ENABLE_THREADS=OFF",
  ]

  cmake_libcxx_kernel_cmd = [
    "cmake",
    os.path.join("..", "..", libcxx_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % cxx_kernel_install_path,
    "-DLIBCXX_HAS_MUSL_LIBC=ON",
    "-DLIBCXX_ENABLE_EXCEPTIONS=OFF",
    "-DLIBCXX_ENABLE_SHARED=OFF",
    "-DLIBCXX_ENABLE_STDIN=OFF",
    "-DLIBCXX_ENABLE_STDOUT=OFF",
    "-DLIBCXX_HAS_EXTERNAL_THREAD_API=ON",
    "-DLIBCXX_HAS_PTHREAD_API=OFF",
    "-DLIBCXX_CXX_ABI=libcxxabi",
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLIBCXX_CXX_ABI_INCLUDE_PATHS=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
    "-DLIBCXX_ENABLE_FILESYSTEM=OFF",
  ]

  cmake_libcxxabi_user_cmd = [
    "cmake",
    os.path.join("..", "..", libcxxabi_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % libcxxabi_user_install_path,
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLIBCXXABI_ENABLE_ASSERTIONS=ON",
    "-DCMAKE_BUILD_TYPE=RELEASE",
    "-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF",
    "-DLIBCXXABI_ENABLE_SHARED=OFF",
    "-DLIBCXXABI_ENABLE_STATIC=ON",
    "-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON",
    "-DLIBCXXABI_ENABLE_PIC=OFF",
    "-DLIBCXXABI_LIBCXX_INCLUDE_DIRS=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
    "-DLIBCXXABI_LIBCXX_INCLUDES=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
  ]

  cmake_libunwind_user_cmd = [
    "cmake",
    os.path.join("..", "..", libunwind_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % unwind_user_install_path,
    "-DLIBUNWIND_ENABLE_SHARED=OFF",
    "-DLIBUNWIND_ENABLE_STATIC=ON",
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLLVM_ENABLE_LIBCXX=ON",
    "-DLIBUNWIND_ENABLE_ASSERTIONS=ON",
    "-DCMAKE_BUILD_TYPE=RELEASE",
    "-DLIBUNWIND_ENABLE_THREADS=ON",
  ]

  cmake_libcxx_user_cmd = [
    "cmake",
    os.path.join("..", "..", libcxx_path),
    "-DLLVM_PATH=%s" % llvm_path,
    "-DCMAKE_INSTALL_PREFIX=%s" % cxx_user_install_path,
    "-DLIBCXX_HAS_MUSL_LIBC=ON",
    "-DLIBCXX_ENABLE_EXCEPTIONS=ON",
    "-DLIBCXX_ENABLE_SHARED=OFF",
    "-DLIBCXX_ENABLE_STDIN=ON",
    "-DLIBCXX_ENABLE_STDOUT=ON",
    "-DLIBCXX_HAS_PTHREAD_API=ON",
    "-DLIBCXX_CXX_ABI=libcxxabi",
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DLIBCXX_CXX_ABI_INCLUDE_PATHS=%s" % os.path.join("..", "..", libcxxabi_path, "include"),
    "-DLIBCXX_ENABLE_FILESYSTEM=OFF",
  ]

  cmake_compiler_rt_user_cmd = [
    "cmake",
    os.path.join("..", "..", compiler_rt_path),
    "-DCMAKE_INSTALL_PREFIX=%s" % compiler_rt_user_install_path,
    "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
    "-DCMAKE_C_COMPILER=/usr/bin/clang",
    "-DCMAKE_BUILD_TYPE=RELEASE",
    "-DCOMPILER_RT_BAREMETAL_BUILD=ON",
    "-DCOMPILER_RT_BUILD_SANITIZERS=OFF",
    "-DSANITIZER_USE_STATIC_CXX_ABI=ON",
    "-DDEFAULT_SANITIZER_USE_STATIC_LLVM_UNWINDER=ON",
    "-DCOMPILER_RT_BUILD_XRAY=OFF",
    "-DCOMPILER_RT_BUILD_LIBFUZZER=OFF",
    "-DCOMPILER_RT_BUILD_PROFILE=OFF",
    "-DCOMPILER_RT_DEFAULT_TARGET_TRIPLE=x86_64-none-elf",
  ]

  compiler_rt_build_lib_cmd = [
    'ar s',
    '-r',
    os.path.join(compiler_rt_user_install_path, "lib", "libc++start.a"),
    os.path.join(compiler_rt_user_install_path, "lib", "linux", "*.o"),
  ]

  # KERNEL MODE LIBRARIES

  os.environ["CXXFLAGS"] = " ".join(core_cxx_flags + kernel_mode_flags)
  os.environ["CFLAGS"] = " ".join(core_c_flags + kernel_mode_flags)

  simple_build("LIBUNWIND (KERNEL MODE)", "output/kernel_libunwind", cmake_libunwind_kernel_cmd)
    # For some reason this doesn't install the headers, so copy them manually
  with cd("output/kernel_libunwind"):
    os.makedirs(os.path.join(unwind_kernel_install_path, "include"), exist_ok = True)
    shutil.copytree(os.path.join("..", "..", libunwind_path, "include"),
                    os.path.join(unwind_kernel_install_path, "include"),
                    dirs_exist_ok = True)

  simple_build("LIBCXXABI (KERNEL MODE)", "output/kernel_libcxxabi", cmake_libcxxabi_kernel_cmd)

  simple_build("LIBCXX (KERNEL MODE)", "output/kernel_libcxx", cmake_libcxx_kernel_cmd)

  # USER MODE LIBRARIES

  os.environ["CXXFLAGS"] = " ".join(core_cxx_flags + user_mode_flags)
  os.environ["CFLAGS"] = " ".join(core_c_flags + user_mode_flags)

  simple_build("LIBUNWIND (USER MODE)", "output/user_libunwind", cmake_libunwind_user_cmd)
  # For some reason this doesn't install the headers, so copy them manually
  with cd("output/user_libunwind"):
    os.makedirs(os.path.join(unwind_user_install_path, "include"), exist_ok = True)
    shutil.copytree(os.path.join("..", "..", libunwind_path, "include"),
                    os.path.join(unwind_user_install_path, "include"),
                    dirs_exist_ok = True)

  simple_build("LIBCXXABI (USER MODE)", "output/user_libcxxabi", cmake_libcxxabi_user_cmd)

  os.environ["CXXFLAGS"] = " ".join(core_cxx_flags + user_mode_flags)
  os.environ["CFLAGS"] = " ".join(core_c_flags + user_mode_flags)
  simple_build("LIBCXX (USER MODE)", "output/user_libcxx", cmake_libcxx_user_cmd)

  simple_build("COMPILER-RT (USER MODE)", "output/user_compiler-rt", cmake_compiler_rt_user_cmd)
  # We then bundle up some of the output to make it easier for the kernel to link.
  with cd("output/user_compiler-rt"):
    print("(Creating archive)")
    os.system(" ".join(compiler_rt_build_lib_cmd))


# An easy way to wrap CD calls so they get reverted upon exiting or exceptions.
#
# Cheekily taken from this Stack Overflow answer:
# https://stackoverflow.com/questions/431684/how-do-i-change-directory-cd-in-python/24176022#24176022
@contextmanager
def cd(newdir):
  prevdir = os.getcwd()
  os.chdir(os.path.expanduser(newdir))
  try:
    yield
  finally:
    os.chdir(prevdir)

def simple_build(name, path, flags):
  print ("")
  print ("--- BUILD " + name + " ---")
  with cd(path):
    if os.system(" ".join(flags)) != 0:
      raise ChildProcessError("Failed to cmake " + name)

    if os.system("make") != 0:
      raise ChildProcessError("Failed to make " + name)

    if os.system("make install") != 0:
      raise ChildProcessError("Failed to install " + name)

def regenerate_config(config, cmd_line_args):
  args_dict = vars(cmd_line_args)

  if "PATHS" not in cfg.sections():
    cfg["PATHS"] = { }

  paths = config["PATHS"]

  populate_field(paths, args_dict, "kernel_base", "Kernel source base directory")
  populate_field(paths, args_dict, "sys_image_root", "Azalea system image root directory")
  populate_field(paths, args_dict, "llvm_base", "LLVM project llvm source folder")

def populate_field(config_section, args_dict, cfg_name, human_name):
  if (cfg_name in args_dict) and args_dict[cfg_name]:
    config_section[cfg_name] = args_dict[cfg_name]
  if cfg_name not in config_section:
    config_section[cfg_name] = prompt_for_value(human_name)

def prompt_for_value(field_name):
  prompt_str = "Enter the following: " + field_name + "\n"
  return input(prompt_str)

if __name__ == "__main__":
  try:
    argp = argparse.ArgumentParser(description = "Project Azalea Builder helper")
    argp.add_argument("--kernel_base", type = str, help = "Location of the base of the Azalea kernel source code tree")
    argp.add_argument("--sys_image_root", type = str, help = "Root of the Azalea system image's filesystem")
    argp.add_argument("--config_file", type = str, default = "output/libcxx_config.ini", help = "Config file location")
    argp.add_argument("--llvm_base", type = str, help = "Location of llvm-project's llvm folder")
    args = argp.parse_args()

    flags = "r" if os.path.exists(args.config_file) else "a+"
    cfg_file = open(args.config_file, flags)
    cfg = configparser.ConfigParser()
    cfg.read_file(cfg_file)
    cfg_file.close()

    regenerate_config(cfg, args)
    main(cfg)

    cfg_file = open(args.config_file, "w")
    cfg.write(cfg_file)
    cfg_file.close()

  except KeyboardInterrupt:
    print ("Build interrupted")
