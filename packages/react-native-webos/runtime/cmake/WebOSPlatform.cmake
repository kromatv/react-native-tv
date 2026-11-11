# What the runtime links from the TV itself. Only a library every webOS release
# carries under the same SONAME may be named, since one executable serves them
# all (docs/reference/webos-native-toolchain.md, "Safe to link from the TV");
# everything else is compiled in or opened with dlopen. A SONAME is not a
# symbol: scripts/check-tv-binary.ts checks each call against firmware data.

set(RN_WEBOS_TV_SONAMES
  libc.so.6 libm.so.6 libpthread.so.0 libdl.so.2 librt.so.1 ld-linux.so.3
  libEGL.so.1 libGLESv2.so.2 libwayland-client.so.0 libwayland-egl.so libwayland-webos-client.so.1
  libxkbcommon.so.0 libfreetype.so.6 libfontconfig.so.1 libpng16.so.16 libz.so.1 libglib-2.0.so.0
  libluna-service2.so.3 libPmLogLib.so.3 libpbnjson_c.so.2 libpulse.so.0 libasound.so.2
  libsqlite3.so.0 libuuid.so.1)

# Links `target` to libraries of the TV, by the name after `lib`. A library
# outside the list above fails the configure.
function(rn_webos_link_tv_libraries target scope)
  foreach(_name IN LISTS ARGN)
    find_library(_rn_webos_tv_${_name} NAMES ${_name} NO_CACHE)
    if(NOT _rn_webos_tv_${_name})
      message(FATAL_ERROR "The webOS SDK has no lib${_name}.so")
    endif()
    execute_process(
      COMMAND "${WEBOS_LLVM_READELF}" --dynamic "${_rn_webos_tv_${_name}}"
      OUTPUT_VARIABLE _dynamic COMMAND_ERROR_IS_FATAL ANY)
    if(NOT _dynamic MATCHES "\\(SONAME\\)[^\n]*\\[([^]]+)\\]")
      message(FATAL_ERROR "${_rn_webos_tv_${_name}} has no SONAME")
    endif()
    if(NOT CMAKE_MATCH_1 IN_LIST RN_WEBOS_TV_SONAMES)
      message(FATAL_ERROR
        "lib${_name}.so is ${CMAKE_MATCH_1}, which not every webOS release has under that name: "
        "compile the library in, or open it with dlopen.")
    endif()
    target_link_libraries(${target} ${scope} "${_rn_webos_tv_${_name}}")
  endforeach()
endfunction()

# Fails the build when the linked `target` needs a library outside the list:
# a dependency's own build files can add one that no call above saw.
function(rn_webos_check_tv_libraries target)
  list(JOIN RN_WEBOS_TV_SONAMES "," _allowed)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DREADELF=${WEBOS_LLVM_READELF}" "-DBINARY=$<TARGET_FILE:${target}>" "-DALLOWED=${_allowed}"
      -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CheckNeededLibraries.cmake"
    VERBATIM)
endfunction()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../compat" "${CMAKE_BINARY_DIR}/compat")

# OpenGL ES for Skia, and zlib for libcurl, which finds the same file itself.
# What SDL needs of the TV (Wayland, xkbcommon, the Luna bus) it opens with
# dlopen.
add_library(rn_webos_platform INTERFACE)
rn_webos_link_tv_libraries(rn_webos_platform INTERFACE EGL GLESv2 z)
target_link_libraries(rn_webos_platform INTERFACE rn_webos_glibc_compat)
