# wayland-scanner for the build machine: it turns the protocol XML SDL carries
# into C while a TV build compiles on a Mac, which has none. Wayland's scanner
# is two C files and expat, here the expat Skia pins.

cmake_minimum_required(VERSION 3.24)
project(rn_webos_wayland_scanner C)

foreach(_dir WAYLAND_SOURCE_DIR EXPAT_SOURCE_DIR EXPAT_CONFIG_DIR)
  if(NOT IS_DIRECTORY "${${_dir}}")
    message(FATAL_ERROR "${_dir} must be a directory, got '${${_dir}}'")
  endif()
endforeach()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/config.h" "")
add_executable(wayland-scanner
  "${WAYLAND_SOURCE_DIR}/src/scanner.c"
  "${WAYLAND_SOURCE_DIR}/src/wayland-util.c"
  "${EXPAT_SOURCE_DIR}/xmlparse.c"
  "${EXPAT_SOURCE_DIR}/xmlrole.c"
  "${EXPAT_SOURCE_DIR}/xmltok.c")
target_include_directories(wayland-scanner PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}" "${WAYLAND_SOURCE_DIR}/src" "${EXPAT_SOURCE_DIR}" "${EXPAT_CONFIG_DIR}")
target_compile_definitions(wayland-scanner PRIVATE XML_STATIC)
target_compile_options(wayland-scanner PRIVATE -w)
