# What the runtime and the libraries compiled into it call beyond glibc 2.12.2,
# the C library of the webOS SDK, which is older than any TV's
# (docs/reference/webos-native-toolchain.md, section 4). include/ declares what
# its headers lack and is on every compile's include path (the toolchain file).
# A call with no definition here fails the link: the executable is linked with
# --no-undefined against that glibc.

file(GLOB _compat_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(rn_webos_glibc_compat STATIC ${_compat_sources})
target_compile_options(rn_webos_glibc_compat PRIVATE -O2 -Wall -Wextra -Werror -ffunction-sections)
