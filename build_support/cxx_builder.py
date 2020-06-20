#!/usr/bin/python3.8
# Main script for azalea_libcxx_builder
#
# This scripts builds kernel and user-mode versions of libcxx that are customised for use with Project Azalea. It also
# builds libunwind for use within the kernel.
#
# Requires python3.8 or newer.

import os

class cxx_data:
  def __init__(self, config):
    self.llvm_path = config["PATHS"]["llvm_base"]
    self.libcxx_path = os.path.join(self.llvm_path, "libcxx")
    self.libcxxabi_path = os.path.join(self.llvm_path, "libcxxabi")
    self.libunwind_path = os.path.join(self.llvm_path, "libunwind")
    self.compiler_rt_path = os.path.join(self.llvm_path, "compiler-rt")

    self.cxx_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx-kernel"))
    self.unwind_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind-kernel"))
    self.libcxxabi_kernel_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxxabi-kernel"))

    self.cxx_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx"))
    self.unwind_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind"))
    self.libcxxabi_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxxabi"))
    self.compiler_rt_user_install_path = os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "compiler-rt"))

    # Main continues after these lists.
    self.core_c_flags = [
      "-fno-threadsafe-statics",
      "-nostdinc",
      "-mcmodel=large",
      "-funwind-tables",
      "-isystem %s" % os.path.abspath(os.path.join(self.libcxx_path, "src")),
      "-isystem %s" % os.path.abspath(os.path.join(self.libcxx_path, "include")),
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libc", "include")),
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "kernel", "include")),
    ]

    self.core_cxx_flags = self.core_c_flags + [
      "-nostdinc++",
      "-std=c++17",
    ]

    self.kernel_mode_flags = [
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["kernel_base"], "kernel", "include")),
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind-kernel", "include")),
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libcxx-kernel", "include", "c++", "v1")),
      "-fno-use-cxa-atexit",
      "-D _LIBUNWIND_IS_BAREMETAL",
      '-D __AZ_KERNEL__',
    ]

    self.user_mode_flags = [
      "-isystem %s" % os.path.abspath(os.path.join(config["PATHS"]["sys_image_root"], "apps", "developer", "libunwind", "include")),
    ]

    self.cmake_libcxxabi_kernel_cmd = [
      "cmake",
      os.path.join("..", "..", self.libcxxabi_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.libcxxabi_kernel_install_path,
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
      "-DLIBCXXABI_LIBCXX_INCLUDE_DIRS=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
      "-DLIBCXXABI_LIBCXX_INCLUDES=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
    ]

    self.cmake_libunwind_kernel_cmd = [
      "cmake",
      os.path.join("..", "..", self.libunwind_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.unwind_kernel_install_path,
      "-DLIBUNWIND_ENABLE_SHARED=OFF",
      "-DLIBUNWIND_ENABLE_STATIC=ON",
      "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
      "-DCMAKE_C_COMPILER=/usr/bin/clang",
      "-DLLVM_ENABLE_LIBCXX=ON",
      "-DLIBUNWIND_ENABLE_ASSERTIONS=OFF",
      "-DCMAKE_BUILD_TYPE=RELEASE",
      "-DLIBUNWIND_ENABLE_THREADS=OFF",
    ]

    self.cmake_libcxx_kernel_cmd = [
      "cmake",
      os.path.join("..", "..", self.libcxx_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.cxx_kernel_install_path,
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
      "-DLIBCXX_CXX_ABI_INCLUDE_PATHS=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
      "-DLIBCXX_ENABLE_FILESYSTEM=OFF",
    ]

    self.cmake_libcxxabi_user_cmd = [
      "cmake",
      os.path.join("..", "..", self.libcxxabi_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.libcxxabi_user_install_path,
      "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
      "-DCMAKE_C_COMPILER=/usr/bin/clang",
      "-DLIBCXXABI_ENABLE_ASSERTIONS=ON",
      "-DCMAKE_BUILD_TYPE=RELEASE",
      "-DLIBCXXABI_ENABLE_NEW_DELETE_DEFINITIONS=OFF",
      "-DLIBCXXABI_ENABLE_SHARED=OFF",
      "-DLIBCXXABI_ENABLE_STATIC=ON",
      "-DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON",
      "-DLIBCXXABI_ENABLE_PIC=OFF",
      "-DLIBCXXABI_LIBCXX_INCLUDE_DIRS=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
      "-DLIBCXXABI_LIBCXX_INCLUDES=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
    ]

    self.cmake_libunwind_user_cmd = [
      "cmake",
      os.path.join("..", "..", self.libunwind_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.unwind_user_install_path,
      "-DLIBUNWIND_ENABLE_SHARED=OFF",
      "-DLIBUNWIND_ENABLE_STATIC=ON",
      "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
      "-DCMAKE_C_COMPILER=/usr/bin/clang",
      "-DLLVM_ENABLE_LIBCXX=ON",
      "-DLIBUNWIND_ENABLE_ASSERTIONS=ON",
      "-DCMAKE_BUILD_TYPE=RELEASE",
      "-DLIBUNWIND_ENABLE_THREADS=ON",
    ]

    self.cmake_libcxx_user_cmd = [
      "cmake",
      os.path.join("..", "..", self.libcxx_path),
      "-DLLVM_PATH=%s" % self.llvm_path,
      "-DCMAKE_INSTALL_PREFIX=%s" % self.cxx_user_install_path,
      "-DLIBCXX_HAS_MUSL_LIBC=ON",
      "-DLIBCXX_ENABLE_EXCEPTIONS=ON",
      "-DLIBCXX_ENABLE_SHARED=OFF",
      "-DLIBCXX_ENABLE_STDIN=ON",
      "-DLIBCXX_ENABLE_STDOUT=ON",
      "-DLIBCXX_HAS_PTHREAD_API=ON",
      "-DLIBCXX_CXX_ABI=libcxxabi",
      "-DCMAKE_CXX_COMPILER=/usr/bin/clang++",
      "-DCMAKE_C_COMPILER=/usr/bin/clang",
      "-DLIBCXX_CXX_ABI_INCLUDE_PATHS=%s" % os.path.join("..", "..", self.libcxxabi_path, "include"),
      "-DLIBCXX_ENABLE_FILESYSTEM=OFF",
    ]

    self.cmake_compiler_rt_user_cmd = [
      "cmake",
      os.path.join("..", "..", self.compiler_rt_path),
      "-DCMAKE_INSTALL_PREFIX=%s" % self.compiler_rt_user_install_path,
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

    self.compiler_rt_build_lib_cmd = [
      'ar s',
      '-r',
      os.path.join(self.compiler_rt_user_install_path, "lib", "libc++start.a"),
      os.path.join(self.compiler_rt_user_install_path, "lib", "linux", "*.o"),
    ]
