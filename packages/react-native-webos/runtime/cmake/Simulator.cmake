# The build for simulator/: aarch64 Linux standing in for a TV. What a TV's
# sysroot provides comes from the distribution here: Mesa's OpenGL ES. Skia,
# SDL, libcurl and Mbed TLS are built from the same sources as for a TV.

if(CMAKE_CROSSCOMPILING OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "RN_WEBOS_SIMULATOR builds natively on Linux, inside the simulator's VM.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(RN_WEBOS_SIMULATOR_GLES REQUIRED IMPORTED_TARGET egl glesv2)

add_library(rn_webos_platform INTERFACE)
target_link_libraries(rn_webos_platform INTERFACE PkgConfig::RN_WEBOS_SIMULATOR_GLES)
