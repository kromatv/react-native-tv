# wayland-scanner, which SDL's Wayland backend runs on the build machine. The
# webOS SDK's Wayland is 1.11 and its own scanner 1.15: a scanner from 1.20 on
# writes calls to wl_proxy_marshal_flags, which those headers do not declare
# (docs/reference/webos-native-toolchain.md, section 6). So the pinned 1.15 is
# compiled for this machine once, into the cache, whatever is installed.
# Sets WAYLAND_SCANNER, the variable SDL looks the program up in.

string(JSON _wayland_version GET "${_rn_webos_dependencies}" wayland version)
string(JSON _expat_pin GET "${_skia_pins}" expat)
string(SHA256 _scanner_key "${_wayland_version} ${_expat_pin}")
string(SUBSTRING "${_scanner_key}" 0 12 _scanner_key)
set(_scanner_dir "${RN_WEBOS_CACHE_DIR}/wayland-scanner/${_wayland_version}-${_scanner_key}")
set(_scanner "${_scanner_dir}/wayland-scanner${CMAKE_HOST_EXECUTABLE_SUFFIX}")

if(NOT EXISTS "${_scanner}")
  message(STATUS "Building wayland-scanner ${_wayland_version} for this machine")
  file(LOCK "${_scanner_dir}.lock" GUARD FILE TIMEOUT 600)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      -S "${CMAKE_CURRENT_LIST_DIR}/../wayland-scanner" -B "${_scanner_dir}" -G "${CMAKE_GENERATOR}"
      -DCMAKE_BUILD_TYPE=Release
      "-DWAYLAND_SOURCE_DIR=${wayland_SOURCE_DIR}"
      "-DEXPAT_SOURCE_DIR=${RN_WEBOS_SKIA_SOURCE_DIR}/third_party/externals/expat/expat/lib"
      "-DEXPAT_CONFIG_DIR=${RN_WEBOS_SKIA_SOURCE_DIR}/third_party/expat/include/expat_config"
    OUTPUT_QUIET
    COMMAND_ERROR_IS_FATAL ANY)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_scanner_dir}" --target wayland-scanner
    OUTPUT_QUIET
    COMMAND_ERROR_IS_FATAL ANY)
  file(LOCK "${_scanner_dir}.lock" RELEASE)
endif()
set(WAYLAND_SCANNER "${_scanner}" CACHE FILEPATH "wayland-scanner for the build machine" FORCE)
