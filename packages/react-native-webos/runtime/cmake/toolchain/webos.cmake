# Cross-compiles for a Samsung webOS TV with LLVM (clang + lld) against the
# rootstrap and the GCC runtime that webOS Studio installs.
#
#   -DCMAKE_TOOLCHAIN_FILE=<this file>
#   -DWEBOS_ARCH=x86_64|aarch64|armv7l     x86_64 is the emulator
#   -DWEBOS_PLATFORM_VERSION=9.0           the oldest platform the build runs on
#   -DWEBOS_ROOTSTRAP_VERSION=<version>    default: WEBOS_PLATFORM_VERSION
#   -DWEBOS_ROOTSTRAP_PROFILE=webos        iot-headed before 8.0 (src/webos/target.ts)
#   -DWEBOS_SDK_DIR=<dir>                  default: $WEBOS_SDK_DIR, then ~/webos-studio
#   -DWEBOS_LLVM_DIR=<dir>                 default: $WEBOS_LLVM_DIR, Homebrew, then PATH
#
# See docs/toolchain.md for why this is clang and not Samsung's GCC.

set(CMAKE_SYSTEM_NAME Linux)

set(WEBOS_ARCH "x86_64" CACHE STRING "Target CPU: x86_64 (emulator), aarch64 or armv7l")
set_property(CACHE WEBOS_ARCH PROPERTY STRINGS x86_64 aarch64 armv7l)
set(WEBOS_PLATFORM_VERSION "9.0" CACHE STRING "Oldest webOS platform the build runs on")
set(WEBOS_ROOTSTRAP_VERSION "${WEBOS_PLATFORM_VERSION}" CACHE STRING "webOS version of the rootstrap")
set(WEBOS_ROOTSTRAP_PROFILE "webos" CACHE STRING "webOS Studio profile of the rootstrap")
set_property(CACHE WEBOS_ROOTSTRAP_PROFILE PROPERTY STRINGS webos iot-headed)

if(DEFINED ENV{WEBOS_SDK_DIR})
  set(WEBOS_SDK_DIR "$ENV{WEBOS_SDK_DIR}" CACHE PATH "webOS Studio installation")
else()
  set(WEBOS_SDK_DIR "$ENV{HOME}/webos-studio" CACHE PATH "webOS Studio installation")
endif()
if(DEFINED ENV{WEBOS_LLVM_DIR})
  set(WEBOS_LLVM_DIR "$ENV{WEBOS_LLVM_DIR}" CACHE PATH "Directory holding clang and ld.lld")
endif()

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  WEBOS_ARCH WEBOS_PLATFORM_VERSION WEBOS_ROOTSTRAP_VERSION WEBOS_ROOTSTRAP_PROFILE WEBOS_SDK_DIR
  WEBOS_LLVM_DIR WEBOS_USE_RAW_ROOTSTRAP)

if(WEBOS_ARCH STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(WEBOS_TRIPLE x86_64-webos-linux-gnu)
  set(_webos_gcc_dir_prefix x86_64-linux-gnu)
  set(_webos_rootstrap emulator64)
  set(WEBOS_LIBDIR lib64)
elseif(WEBOS_ARCH STREQUAL "aarch64")
  set(CMAKE_SYSTEM_PROCESSOR aarch64)
  set(WEBOS_TRIPLE aarch64-webos-linux-gnu)
  set(_webos_gcc_dir_prefix aarch64-linux-gnu)
  set(_webos_rootstrap device64)
  set(WEBOS_LIBDIR lib64)
elseif(WEBOS_ARCH STREQUAL "armv7l")
  set(CMAKE_SYSTEM_PROCESSOR armv7l)
  set(WEBOS_TRIPLE arm-webos-linux-gnueabi)
  set(_webos_gcc_dir_prefix arm-linux-gnueabi)
  set(_webos_rootstrap device)
  set(WEBOS_LIBDIR lib)
  # The configuration Samsung's own GCC is built with (its configargs.h).
  set(_webos_arch_flags "-march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=vfpv3-d16")
else()
  message(FATAL_ERROR "WEBOS_ARCH must be x86_64, aarch64 or armv7l, got '${WEBOS_ARCH}'")
endif()

file(GLOB _webos_gcc_dirs LIST_DIRECTORIES true "${WEBOS_SDK_DIR}/tools/${_webos_gcc_dir_prefix}-gcc-*")
list(SORT _webos_gcc_dirs COMPARE NATURAL ORDER DESCENDING)
list(GET _webos_gcc_dirs 0 WEBOS_GCC_DIR)
file(GLOB _webos_gcc_install_dirs LIST_DIRECTORIES true "${WEBOS_GCC_DIR}/lib/gcc/${WEBOS_TRIPLE}/*")
list(GET _webos_gcc_install_dirs 0 WEBOS_GCC_INSTALL_DIR)
if(NOT EXISTS "${WEBOS_GCC_INSTALL_DIR}/crtbegin.o")
  message(FATAL_ERROR
    "No GCC runtime for ${WEBOS_TRIPLE} under ${WEBOS_SDK_DIR}/tools. "
    "Install the Native toolchain from the webOS Studio Package Manager.")
endif()

set(WEBOS_ROOTSTRAP_NAME "${WEBOS_ROOTSTRAP_PROFILE}-${WEBOS_ROOTSTRAP_VERSION}-${_webos_rootstrap}.core")
set(WEBOS_ROOTSTRAP
  "${WEBOS_SDK_DIR}/platforms/webos-${WEBOS_ROOTSTRAP_VERSION}/${WEBOS_ROOTSTRAP_PROFILE}/rootstraps/${WEBOS_ROOTSTRAP_NAME}")
if(NOT IS_DIRECTORY "${WEBOS_ROOTSTRAP}")
  message(FATAL_ERROR
    "Rootstrap not found: ${WEBOS_ROOTSTRAP}. Run: rn-webos setup --arch ${WEBOS_ARCH} --platform ${WEBOS_PLATFORM_VERSION}")
endif()

if(DEFINED ENV{RN_WEBOS_CACHE_DIR})
  set(RN_WEBOS_CACHE_DIR "$ENV{RN_WEBOS_CACHE_DIR}")
elseif(DEFINED ENV{XDG_CACHE_HOME})
  set(RN_WEBOS_CACHE_DIR "$ENV{XDG_CACHE_HOME}/react-native-webos")
else()
  set(RN_WEBOS_CACHE_DIR "$ENV{HOME}/.cache/react-native-webos")
endif()

# The rootstrap's own stubs are unreadable to lld, and its glibc predates GCC
# 14's runtime: `rn-webos setup` builds a mirror that fixes both (docs/toolchain.md).
option(WEBOS_USE_RAW_ROOTSTRAP "Compile against the rootstrap itself, without linking" OFF)
if(WEBOS_USE_RAW_ROOTSTRAP)
  set(WEBOS_SYSROOT "${WEBOS_ROOTSTRAP}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
else()
  set(WEBOS_SYSROOT "${RN_WEBOS_CACHE_DIR}/sysroots/${WEBOS_ROOTSTRAP_NAME}")
  if(NOT EXISTS "${WEBOS_SYSROOT}/.rn-webos-sysroot.json")
    message(FATAL_ERROR
      "No linkable sysroot for ${WEBOS_ROOTSTRAP_NAME}. "
      "Run: rn-webos setup --arch ${WEBOS_ARCH} --platform ${WEBOS_PLATFORM_VERSION}")
  endif()
endif()

set(_webos_llvm_hints "${WEBOS_LLVM_DIR}" /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
set(_webos_lld_hints "${WEBOS_LLVM_DIR}" /opt/homebrew/opt/lld/bin /usr/local/opt/lld/bin /opt/homebrew/opt/llvm/bin)
find_program(WEBOS_CLANG clang HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_CLANGXX clang++ HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_AR llvm-ar HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_RANLIB llvm-ranlib HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
find_program(WEBOS_LLVM_STRIP llvm-strip HINTS ${_webos_llvm_hints} NO_CACHE REQUIRED)
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

set(_webos_common_flags "--gcc-install-dir=${WEBOS_GCC_INSTALL_DIR} -fPIC ${_webos_arch_flags}")
set(CMAKE_C_FLAGS_INIT "${_webos_common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_webos_common_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_webos_common_flags}")

# A TV's system libstdc++ is as old as its firmware, so the runtime carries its
# own: a set from 2022 refuses a library that needs GCC 14's.
set(_webos_link_flags "--ld-path=${WEBOS_LLD} -static-libstdc++ -static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_webos_link_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_webos_link_flags} -Wl,--no-undefined")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_webos_link_flags}")
# Static like libstdc++: armv7 has no instructions for some atomic widths React Native uses.
set(CMAKE_C_STANDARD_LIBRARIES_INIT "-l:libatomic.a")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "-l:libatomic.a")

set(ENV{PKG_CONFIG_DIR} "")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${WEBOS_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
  "${WEBOS_SYSROOT}/usr/${WEBOS_LIBDIR}/pkgconfig:${WEBOS_SYSROOT}/usr/share/pkgconfig")
