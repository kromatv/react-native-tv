# Cross-compiles for a Samsung Tizen TV with LLVM (clang + lld) against the
# rootstrap and the GCC runtime that Tizen Studio installs.
#
#   -DCMAKE_TOOLCHAIN_FILE=<this file>
#   -DTIZEN_ARCH=x86_64|aarch64|armv7l     x86_64 is the emulator
#   -DTIZEN_PLATFORM_VERSION=9.0           the oldest platform the build runs on
#   -DTIZEN_SDK_DIR=<dir>                  default: $TIZEN_SDK_DIR, then ~/tizen-studio
#   -DTIZEN_LLVM_DIR=<dir>                 default: $TIZEN_LLVM_DIR, Homebrew, then PATH
#
# See docs/toolchain.md for why this is clang and not Samsung's GCC.

set(CMAKE_SYSTEM_NAME Linux)

set(TIZEN_ARCH "x86_64" CACHE STRING "Target CPU: x86_64 (emulator), aarch64 or armv7l")
set_property(CACHE TIZEN_ARCH PROPERTY STRINGS x86_64 aarch64 armv7l)
set(TIZEN_PLATFORM_VERSION "9.0" CACHE STRING "Oldest Tizen platform the build runs on")

if(DEFINED ENV{TIZEN_SDK_DIR})
  set(TIZEN_SDK_DIR "$ENV{TIZEN_SDK_DIR}" CACHE PATH "Tizen Studio installation")
else()
  set(TIZEN_SDK_DIR "$ENV{HOME}/tizen-studio" CACHE PATH "Tizen Studio installation")
endif()
if(DEFINED ENV{TIZEN_LLVM_DIR})
  set(TIZEN_LLVM_DIR "$ENV{TIZEN_LLVM_DIR}" CACHE PATH "Directory holding clang and ld.lld")
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  TIZEN_ARCH TIZEN_PLATFORM_VERSION TIZEN_SDK_DIR TIZEN_LLVM_DIR TIZEN_USE_RAW_ROOTSTRAP)

if(TIZEN_ARCH STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(TIZEN_TRIPLE x86_64-tizen-linux-gnu)
  set(_tizen_gcc_dir_prefix x86_64-linux-gnu)
  set(_tizen_rootstrap emulator64)
  set(TIZEN_LIBDIR lib64)
elseif(TIZEN_ARCH STREQUAL "aarch64")
  set(CMAKE_SYSTEM_PROCESSOR aarch64)
  set(TIZEN_TRIPLE aarch64-tizen-linux-gnu)
  set(_tizen_gcc_dir_prefix aarch64-linux-gnu)
  set(_tizen_rootstrap device64)
  set(TIZEN_LIBDIR lib64)
elseif(TIZEN_ARCH STREQUAL "armv7l")
  set(CMAKE_SYSTEM_PROCESSOR armv7l)
  set(TIZEN_TRIPLE arm-tizen-linux-gnueabi)
  set(_tizen_gcc_dir_prefix arm-linux-gnueabi)
  set(_tizen_rootstrap device)
  set(TIZEN_LIBDIR lib)
  # The configuration Samsung's own GCC is built with (its configargs.h).
  set(_tizen_arch_flags "-march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=vfpv3-d16")
else()
  message(FATAL_ERROR "TIZEN_ARCH must be x86_64, aarch64 or armv7l, got '${TIZEN_ARCH}'")
endif()

file(GLOB _tizen_gcc_dirs LIST_DIRECTORIES true "${TIZEN_SDK_DIR}/tools/${_tizen_gcc_dir_prefix}-gcc-*")
list(SORT _tizen_gcc_dirs COMPARE NATURAL ORDER DESCENDING)
list(GET _tizen_gcc_dirs 0 TIZEN_GCC_DIR)
file(GLOB _tizen_gcc_install_dirs LIST_DIRECTORIES true "${TIZEN_GCC_DIR}/lib/gcc/${TIZEN_TRIPLE}/*")
list(GET _tizen_gcc_install_dirs 0 TIZEN_GCC_INSTALL_DIR)
if(NOT EXISTS "${TIZEN_GCC_INSTALL_DIR}/crtbegin.o")
  message(FATAL_ERROR
    "No GCC runtime for ${TIZEN_TRIPLE} under ${TIZEN_SDK_DIR}/tools. "
    "Install the Native toolchain from the Tizen Studio Package Manager.")
endif()

set(TIZEN_ROOTSTRAP_NAME "tizen-${TIZEN_PLATFORM_VERSION}-${_tizen_rootstrap}.core")
set(TIZEN_ROOTSTRAP
  "${TIZEN_SDK_DIR}/platforms/tizen-${TIZEN_PLATFORM_VERSION}/tizen/rootstraps/${TIZEN_ROOTSTRAP_NAME}")
if(NOT IS_DIRECTORY "${TIZEN_ROOTSTRAP}")
  message(FATAL_ERROR
    "Rootstrap not found: ${TIZEN_ROOTSTRAP}. Install TIZEN-${TIZEN_PLATFORM_VERSION}-NativeAppDevelopment-CLI "
    "from the Tizen Studio Package Manager.")
endif()

if(DEFINED ENV{RN_TIZEN_CACHE_DIR})
  set(RN_TIZEN_CACHE_DIR "$ENV{RN_TIZEN_CACHE_DIR}")
elseif(DEFINED ENV{XDG_CACHE_HOME})
  set(RN_TIZEN_CACHE_DIR "$ENV{XDG_CACHE_HOME}/react-native-tizen")
else()
  set(RN_TIZEN_CACHE_DIR "$ENV{HOME}/.cache/react-native-tizen")
endif()

# The rootstrap's own stubs are unreadable to lld, and its glibc predates GCC
# 14's runtime: `rn-tizen setup` builds a mirror that fixes both (docs/toolchain.md).
option(TIZEN_USE_RAW_ROOTSTRAP "Compile against the rootstrap itself, without linking" OFF)
if(TIZEN_USE_RAW_ROOTSTRAP)
  set(TIZEN_SYSROOT "${TIZEN_ROOTSTRAP}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
else()
  set(TIZEN_SYSROOT "${RN_TIZEN_CACHE_DIR}/sysroots/${TIZEN_ROOTSTRAP_NAME}")
  if(NOT EXISTS "${TIZEN_SYSROOT}/.rn-tizen-sysroot.json")
    message(FATAL_ERROR
      "No linkable sysroot for ${TIZEN_ROOTSTRAP_NAME}. "
      "Run: rn-tizen setup --arch ${TIZEN_ARCH} --platform ${TIZEN_PLATFORM_VERSION}")
  endif()
endif()

set(_tizen_llvm_hints "${TIZEN_LLVM_DIR}" /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
set(_tizen_lld_hints "${TIZEN_LLVM_DIR}" /opt/homebrew/opt/lld/bin /usr/local/opt/lld/bin /opt/homebrew/opt/llvm/bin)
find_program(TIZEN_CLANG clang HINTS ${_tizen_llvm_hints} NO_CACHE REQUIRED)
find_program(TIZEN_CLANGXX clang++ HINTS ${_tizen_llvm_hints} NO_CACHE REQUIRED)
find_program(TIZEN_LLVM_AR llvm-ar HINTS ${_tizen_llvm_hints} NO_CACHE REQUIRED)
find_program(TIZEN_LLVM_RANLIB llvm-ranlib HINTS ${_tizen_llvm_hints} NO_CACHE REQUIRED)
find_program(TIZEN_LLVM_STRIP llvm-strip HINTS ${_tizen_llvm_hints} NO_CACHE REQUIRED)
find_program(TIZEN_LLD ld.lld HINTS ${_tizen_lld_hints} NO_CACHE REQUIRED)

set(CMAKE_C_COMPILER "${TIZEN_CLANG}")
set(CMAKE_CXX_COMPILER "${TIZEN_CLANGXX}")
set(CMAKE_ASM_COMPILER "${TIZEN_CLANG}")
set(CMAKE_C_COMPILER_TARGET ${TIZEN_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TIZEN_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${TIZEN_TRIPLE})
set(CMAKE_AR "${TIZEN_LLVM_AR}" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${TIZEN_LLVM_RANLIB}" CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP "${TIZEN_LLVM_STRIP}" CACHE FILEPATH "" FORCE)

set(CMAKE_SYSROOT "${TIZEN_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "${TIZEN_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(_tizen_common_flags "--gcc-install-dir=${TIZEN_GCC_INSTALL_DIR} -fPIC ${_tizen_arch_flags}")
set(CMAKE_C_FLAGS_INIT "${_tizen_common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_tizen_common_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_tizen_common_flags}")

# A TV's system libstdc++ is as old as its firmware, so the runtime carries its
# own: a set from 2022 refuses a library that needs GCC 14's.
set(_tizen_link_flags "--ld-path=${TIZEN_LLD} -static-libstdc++ -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_tizen_link_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_tizen_link_flags} -Wl,--no-undefined")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_tizen_link_flags}")
# Static like libstdc++: armv7 has no instructions for some atomic widths React Native uses.
set(CMAKE_C_STANDARD_LIBRARIES_INIT "-l:libatomic.a")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "-l:libatomic.a")

set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${TIZEN_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
  "${TIZEN_SYSROOT}/usr/${TIZEN_LIBDIR}/pkgconfig:${TIZEN_SYSROOT}/usr/share/pkgconfig")
