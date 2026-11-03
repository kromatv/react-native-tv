# Tizen's public native API, read from the rootstrap's own manifest
# (rootstraps/info/<rootstrap>.dev.xml), which is what Tizen Studio builds from.
# The rootstrap's pkg-config files cannot be used: they require packages
# Samsung does not ship in it.
#
#   target_link_tizen_libraries(<target> elementary dlog ...)
#
# links public libraries only. A name outside the manifest fails the configure,
# because the store rejects a package that links a private platform library.

set(_tizen_manifest
  "${TIZEN_SDK_DIR}/platforms/tizen-${TIZEN_PLATFORM_VERSION}/tizen/rootstraps/info/${TIZEN_ROOTSTRAP_NAME}.dev.xml")
if(NOT EXISTS "${_tizen_manifest}")
  message(FATAL_ERROR "Tizen native API manifest not found: ${_tizen_manifest}")
endif()

file(STRINGS "${_tizen_manifest}" _tizen_include_lines REGEX "<include_path>")
set(_tizen_include_dirs "")
foreach(_line IN LISTS _tizen_include_lines)
  string(REGEX REPLACE ".*<include_path>[ \t]*([^< \t]*)[ \t]*</include_path>.*" "\\1" _path "${_line}")
  string(REGEX REPLACE "/$" "" _path "${_path}")
  if(IS_DIRECTORY "${CMAKE_SYSROOT}${_path}")
    list(APPEND _tizen_include_dirs "${CMAKE_SYSROOT}${_path}")
  endif()
endforeach()
list(REMOVE_DUPLICATES _tizen_include_dirs)

file(STRINGS "${_tizen_manifest}" _tizen_library_lines REGEX "<library>")
set(TIZEN_PUBLIC_LIBRARIES "")
foreach(_line IN LISTS _tizen_library_lines)
  string(REGEX REPLACE ".*<library>[ \t]*lib([^<]*)\\.so[ \t]*</library>.*" "\\1" _name "${_line}")
  list(APPEND TIZEN_PUBLIC_LIBRARIES "${_name}")
endforeach()

add_library(TizenNativeAPI INTERFACE)
list(REMOVE_ITEM _tizen_include_dirs "${CMAKE_SYSROOT}/usr/include")
target_include_directories(TizenNativeAPI SYSTEM INTERFACE ${_tizen_include_dirs})

function(target_link_tizen_libraries target)
  foreach(_library IN LISTS ARGN)
    if(NOT _library IN_LIST TIZEN_PUBLIC_LIBRARIES)
      message(FATAL_ERROR "lib${_library}.so is not part of Tizen ${TIZEN_PLATFORM_VERSION}'s public native API.")
    endif()
  endforeach()
  target_link_libraries(${target} PRIVATE TizenNativeAPI ${ARGN})
endfunction()
