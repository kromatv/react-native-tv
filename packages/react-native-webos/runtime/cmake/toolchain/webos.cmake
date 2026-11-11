# Cross-compiles for an LG webOS TV with LLVM (clang and lld) against openlgtv's
# SDK: a glibc 2.12.2 sysroot and GCC's C++ runtime, linked statically
# (docs/reference/webos-native-toolchain.md, "What this means").
#
#   -DCMAKE_TOOLCHAIN_FILE=<this file>
#   -DWEBOS_SDK_DIR=<dir>    the SDK as `rn-webos setup` extracts it; default: $WEBOS_SDK_DIR
#   -DWEBOS_LLVM_DIR=<dir>   holds clang and ld.lld; default: $WEBOS_LLVM_DIR, Homebrew, then PATH
#   -DWEBOS_ARM_MODE=ON      ARM instructions instead of Thumb-2

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)
set(WEBOS_TRIPLE arm-webos-linux-gnueabi)

if(NOT WEBOS_SDK_DIR AND DEFINED ENV{WEBOS_SDK_DIR})
  set(WEBOS_SDK_DIR "$ENV{WEBOS_SDK_DIR}")
endif()
set(WEBOS_SDK_DIR "${WEBOS_SDK_DIR}" CACHE PATH "The extracted webOS SDK (openlgtv buildroot-nc4)")
if(NOT WEBOS_LLVM_DIR AND DEFINED ENV{WEBOS_LLVM_DIR})
  set(WEBOS_LLVM_DIR "$ENV{WEBOS_LLVM_DIR}")
endif()
set(WEBOS_LLVM_DIR "${WEBOS_LLVM_DIR}" CACHE PATH "Directory holding clang and ld.lld")
option(WEBOS_ARM_MODE "Compile ARM instructions instead of Thumb-2" OFF)

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES WEBOS_SDK_DIR WEBOS_LLVM_DIR WEBOS_ARM_MODE)

set(WEBOS_SYSROOT "${WEBOS_SDK_DIR}/${WEBOS_TRIPLE}/sysroot")
file(GLOB _webos_gcc_install_dirs LIST_DIRECTORIES true "${WEBOS_SDK_DIR}/lib/gcc/${WEBOS_TRIPLE}/*")
list(SORT _webos_gcc_install_dirs COMPARE NATURAL ORDER DESCENDING)
list(APPEND _webos_gcc_install_dirs "")
list(GET _webos_gcc_install_dirs 0 WEBOS_GCC_INSTALL_DIR)
if(NOT EXISTS "${WEBOS_SYSROOT}/usr/lib/libc.so" OR NOT EXISTS "${WEBOS_GCC_INSTALL_DIR}/crtbeginS.o")
  message(FATAL_ERROR
    "WEBOS_SDK_DIR ('${WEBOS_SDK_DIR}') holds no ${WEBOS_TRIPLE} sysroot and GCC runtime. Run: rn-webos setup")
endif()

set(_webos_llvm_hints "${WEBOS_LLVM_DIR}" /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
set(_webos_lld_hints "${WEBOS_LLVM_DIR}" /opt/homebrew/opt/lld/bin /usr/local/opt/lld/bin /opt/homebrew/opt/llvm/bin)
find_program(WEBOS_CLANG clang HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_CLANGXX clang++ HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_AR llvm-ar HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_RANLIB llvm-ranlib HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_STRIP llvm-strip HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_READELF llvm-readelf HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLD ld.lld HINTS ${_webos_lld_hints} NO_CACHE REQUIRED)

set(CMAKE_C_COMPILER "${WEBOS_CLANG}")
set(CMAKE_CXX_COMPILER "${WEBOS_CLANGXX}")
set(CMAKE_ASM_COMPILER "${WEBOS_CLANG}")
set(CMAKE_C_COMPILER_TARGET ${WEBOS_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${WEBOS_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${WEBOS_TRIPLE})
set(CMAKE_AR "${WEBOS_LLVM_AR}" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${WEBOS_LLVM_RANLIB}" CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP "${WEBOS_LLVM_STRIP}" CACHE FILEPATH "" FORCE)

set(CMAKE_SYSROOT "${WEBOS_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "${WEBOS_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# The ABI the SDK's GCC is configured for and every TV runs: armv7 with NEON,
# floats passed in core registers. Thumb-2 is what Skia's own build chooses for
# 32-bit ARM.
if(WEBOS_ARM_MODE)
  set(_webos_instruction_set -marm)
else()
  set(_webos_instruction_set -mthumb)
endif()
set(WEBOS_ABI_FLAGS
  "--gcc-install-dir=${WEBOS_GCC_INSTALL_DIR}" -mcpu=cortex-a9 -mfpu=neon-fp16 -mfloat-abi=softfp
  ${_webos_instruction_set})
# runtime/compat/include declares what glibc 2.12.2's headers lack.
get_filename_component(WEBOS_COMPAT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../compat" ABSOLUTE)
set(_webos_compile_flags
  ${WEBOS_ABI_FLAGS} -fPIC "-isystem${WEBOS_COMPAT_DIR}/include"
  -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)
list(JOIN _webos_compile_flags " " _webos_compile_flags)
set(CMAKE_C_FLAGS_INIT "${_webos_compile_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_webos_compile_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_webos_compile_flags}")

# A TV's libstdc++ is as old as its firmware and libatomic.so.1 is missing on
# webOS 1 and 4, so both are linked in. glibc 2.12.2 keeps clock_gettime in librt.
set(_webos_link_flags
  "--ld-path=${WEBOS_LLD} -static-libstdc++ -static-libgcc -pthread -Wl,--no-undefined -Wl,--hash-style=both")
# A C link ignores -static-libstdc++, and clang says so: a configure check that
# links with -Werror, such as SDL's for -fvisibility, would then fail.
string(APPEND _webos_link_flags " -Wno-unused-command-line-argument")
# The SDK's libatomic.a is not position independent, so an executable is linked
# at a fixed address, as the SDK's GCC links one by default.
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_webos_link_flags} -no-pie")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_webos_link_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_webos_link_flags}")
set(_webos_standard_libraries "-l:libatomic.a -Wl,--as-needed -lrt -Wl,--no-as-needed")
set(CMAKE_C_STANDARD_LIBRARIES_INIT "${_webos_standard_libraries}")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "${_webos_standard_libraries}")

set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${WEBOS_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${WEBOS_SYSROOT}/usr/lib/pkgconfig:${WEBOS_SYSROOT}/usr/share/pkgconfig")

# Skia compiles itself with GN, outside CMake: the same compiler, target and
# ABI, as GN arguments (docs/reference/skia-rendering.md, section 4).
set(RN_WEBOS_SKIA_CFLAGS "--target=${WEBOS_TRIPLE}" "--sysroot=${WEBOS_SYSROOT}" ${WEBOS_ABI_FLAGS})
set(_webos_skia_asmflags ${RN_WEBOS_SKIA_CFLAGS})
list(TRANSFORM _webos_skia_asmflags PREPEND "\"")
list(TRANSFORM _webos_skia_asmflags APPEND "\"")
list(JOIN _webos_skia_asmflags ", " _webos_skia_asmflags)
set(RN_WEBOS_SKIA_TARGET_ARGS
  "target_cpu = \"arm\""
  "cc = \"${WEBOS_CLANG}\""
  "cxx = \"${WEBOS_CLANGXX}\""
  "ar = \"${WEBOS_LLVM_AR}\""
  "extra_asmflags = [ ${_webos_skia_asmflags} ]")
