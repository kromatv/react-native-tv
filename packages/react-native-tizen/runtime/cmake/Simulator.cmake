# The build for simulator/: aarch64 Linux standing in for a TV. The Tizen
# native API comes from the distribution's EFL and libcurl plus the shims in
# simulator/shims, under the target and function TizenNativeApi.cmake defines
# for a TV, so host/ builds unchanged.

if(CMAKE_CROSSCOMPILING OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "RN_TIZEN_SIMULATOR builds natively on Linux, inside the simulator's VM.")
endif()

if(NOT RN_TIZEN_CACHE_DIR)
  if(DEFINED ENV{RN_TIZEN_CACHE_DIR})
    set(RN_TIZEN_CACHE_DIR "$ENV{RN_TIZEN_CACHE_DIR}")
  elseif(DEFINED ENV{XDG_CACHE_HOME})
    set(RN_TIZEN_CACHE_DIR "$ENV{XDG_CACHE_HOME}/react-native-tizen")
  else()
    set(RN_TIZEN_CACHE_DIR "$ENV{HOME}/.cache/react-native-tizen")
  endif()
endif()

set(_simulator_dir "${CMAKE_CURRENT_LIST_DIR}/../../simulator")

find_package(PkgConfig REQUIRED)
pkg_check_modules(RN_TIZEN_SIMULATOR_EFL REQUIRED IMPORTED_TARGET elementary ecore ecore-evas ecore-imf ecore-input evas eina eo)
pkg_check_modules(RN_TIZEN_SIMULATOR_CURL REQUIRED IMPORTED_TARGET libcurl)

# Ubuntu's evas.pc names /usr/include/ector-1, which no package installs, and
# CMake refuses an imported target with a missing include directory.
get_target_property(_efl_include_dirs PkgConfig::RN_TIZEN_SIMULATOR_EFL INTERFACE_INCLUDE_DIRECTORIES)
set(_efl_existing_include_dirs "")
foreach(_dir IN LISTS _efl_include_dirs)
  if(IS_DIRECTORY "${_dir}")
    list(APPEND _efl_existing_include_dirs "${_dir}")
  endif()
endforeach()
set_target_properties(PkgConfig::RN_TIZEN_SIMULATOR_EFL PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_efl_existing_include_dirs}")

add_subdirectory("${_simulator_dir}/shims" "${CMAKE_BINARY_DIR}/simulator/shims")

# A TV build links with --no-undefined too (toolchain/tizen.cmake): a host
# source the build misses must fail the link, not the app's start.
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")

add_library(TizenNativeAPI INTERFACE)
target_link_libraries(TizenNativeAPI INTERFACE
  PkgConfig::RN_TIZEN_SIMULATOR_EFL PkgConfig::RN_TIZEN_SIMULATOR_CURL rn-tizen-simulator-shims)

set(RN_TIZEN_SIMULATOR_LIBRARIES
  elementary ecore ecore_evas ecore_imf ecore_input evas eina eo efl-extension dlog curl
  capi-system-info capi-appfw-application capi-appfw-app-common capi-media-player)

function(target_link_tizen_libraries target)
  foreach(_library IN LISTS ARGN)
    if(NOT _library IN_LIST RN_TIZEN_SIMULATOR_LIBRARIES)
      message(FATAL_ERROR "lib${_library}.so has no stand-in in the simulator: add one to simulator/shims.")
    endif()
  endforeach()
  target_link_libraries(${target} PRIVATE TizenNativeAPI)
endfunction()

add_subdirectory("${_simulator_dir}/launcher" "${CMAKE_BINARY_DIR}/simulator/launcher")
