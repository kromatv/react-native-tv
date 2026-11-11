# webOS's public native API, read from the rootstrap's own manifest
# (rootstraps/info/<rootstrap>.dev.xml), which is what webOS Studio builds from.
# The rootstrap's pkg-config files cannot be used: they require packages
# Samsung does not ship in it.
#
#   target_link_webos_libraries(<target> elementary dlog ...)
#
# links public libraries only. A name outside the manifest fails the configure,
# because the store rejects a package that links a private platform library.

set(_webos_manifest
  "${WEBOS_SDK_DIR}/platforms/webos-${WEBOS_ROOTSTRAP_VERSION}/${WEBOS_ROOTSTRAP_PROFILE}/rootstraps/info/${WEBOS_ROOTSTRAP_NAME}.dev.xml")
if(NOT EXISTS "${_webos_manifest}")
  message(FATAL_ERROR "webOS native API manifest not found: ${_webos_manifest}")
endif()

file(STRINGS "${_webos_manifest}" _webos_include_lines REGEX "<include_path>")
set(_webos_include_dirs "")
foreach(_line IN LISTS _webos_include_lines)
  string(REGEX REPLACE ".*<include_path>[ \t]*([^< \t]*)[ \t]*</include_path>.*" "\\1" _path "${_line}")
  string(REGEX REPLACE "/$" "" _path "${_path}")
  if(IS_DIRECTORY "${CMAKE_SYSROOT}${_path}")
    list(APPEND _webos_include_dirs "${CMAKE_SYSROOT}${_path}")
  endif()
endforeach()
list(REMOVE_DUPLICATES _webos_include_dirs)

file(STRINGS "${_webos_manifest}" _webos_library_lines REGEX "<library>")
set(WEBOS_PUBLIC_LIBRARIES "")
foreach(_line IN LISTS _webos_library_lines)
  string(REGEX REPLACE ".*<library>[ \t]*lib([^<]*)\\.so[ \t]*</library>.*" "\\1" _name "${_line}")
  list(APPEND WEBOS_PUBLIC_LIBRARIES "${_name}")
endforeach()

add_library(WebOSNativeAPI INTERFACE)
list(REMOVE_ITEM _webos_include_dirs "${CMAKE_SYSROOT}/usr/include")
target_include_directories(WebOSNativeAPI SYSTEM INTERFACE ${_webos_include_dirs})

function(target_link_webos_libraries target)
  foreach(_library IN LISTS ARGN)
    if(NOT _library IN_LIST WEBOS_PUBLIC_LIBRARIES)
      message(FATAL_ERROR "lib${_library}.so is not part of webOS ${WEBOS_PLATFORM_VERSION}'s public native API.")
    endif()
  endforeach()
  target_link_libraries(${target} PRIVATE WebOSNativeAPI ${ARGN})
endfunction()
