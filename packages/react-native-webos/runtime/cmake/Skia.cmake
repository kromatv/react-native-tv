# Skia, the 2D renderer every view is drawn with, and SkParagraph for text.
# Skia only builds with its own GN build (docs/reference/skia-rendering.md), so
# CMake fetches the pinned sources, writes the GN arguments, and runs Ninja on
# Skia's build directory from a target of its own. Sources and builds live in
# RN_WEBOS_SKIA_ROOT, outside any one runtime build: Skia takes a quarter of an
# hour to compile and does not depend on the app.

set(RN_WEBOS_SKIA_ROOT "${RN_WEBOS_CACHE_DIR}/skia" CACHE PATH
  "Where Skia's sources and builds are kept, shared by every runtime build")
set(RN_WEBOS_SKIA_TARGET_ARGS "" CACHE STRING
  "GN arguments for the target, one per list item; a toolchain file sets them for a TV")
set(RN_WEBOS_SKIA_CFLAGS "" CACHE STRING "Compiler flags Skia's own sources need for the target")

find_program(RN_WEBOS_GN gn REQUIRED)
find_program(RN_WEBOS_NINJA NAMES ninja ninja-build REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Git REQUIRED)

set(_skia_pins_file "${CMAKE_CURRENT_LIST_DIR}/../skia/dependencies.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_skia_pins_file}")
file(READ "${_skia_pins_file}" _skia_pins)
string(SHA256 _skia_pins_id "${_skia_pins}")
string(SUBSTRING "${_skia_pins_id}" 0 12 _skia_pins_id)

file(MAKE_DIRECTORY "${RN_WEBOS_SKIA_ROOT}/src")
file(LOCK "${RN_WEBOS_SKIA_ROOT}/sources.lock" GUARD FILE TIMEOUT 3600)

function(_rn_webos_skia_archive name url sha256 out_var)
  get_filename_component(_file "${url}" NAME)
  set(_archive "${RN_WEBOS_DOWNLOAD_DIR}/skia/${name}-${_file}")
  if(NOT EXISTS "${_archive}")
    message(STATUS "Downloading ${name}")
    file(DOWNLOAD "${url}" "${_archive}.part" EXPECTED_HASH SHA256=${sha256} STATUS _status)
    list(GET _status 0 _code)
    if(NOT _code EQUAL 0)
      message(FATAL_ERROR "Cannot download ${url}: ${_status}")
    endif()
    file(RENAME "${_archive}.part" "${_archive}")
  endif()
  set(${out_var} "${_archive}" PARENT_SCOPE)
endfunction()

# Fetches one pinned source tree into `dir`: an archive checked by its SHA-256,
# or one git commit, whose id is the checksum (Chromium's forks of zlib and
# libjpeg-turbo have no archive that hashes the same twice).
function(_rn_webos_skia_fetch name dir)
  if(EXISTS "${dir}")
    return()
  endif()
  set(_staging "${dir}.part")
  file(REMOVE_RECURSE "${_staging}")
  string(JSON _git ERROR_VARIABLE _no_git GET "${_skia_pins}" ${name} git)
  if(_no_git)
    string(JSON _url GET "${_skia_pins}" ${name} url)
    string(JSON _sha256 GET "${_skia_pins}" ${name} sha256)
    _rn_webos_skia_archive(${name} "${_url}" "${_sha256}" _archive)
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_staging}")
    file(GLOB _top "${_staging}/*")
    file(RENAME "${_top}" "${dir}")
    file(REMOVE_RECURSE "${_staging}")
    return()
  endif()
  string(JSON _commit GET "${_skia_pins}" ${name} commit)
  message(STATUS "Fetching ${name} at ${_commit}")
  execute_process(COMMAND "${GIT_EXECUTABLE}" init -q "${_staging}" COMMAND_ERROR_IS_FATAL ANY)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${_staging}" fetch -q --depth=1 "${_git}" "${_commit}"
    COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${_staging}" checkout -q FETCH_HEAD COMMAND_ERROR_IS_FATAL ANY)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${_staging}" rev-parse HEAD
    OUTPUT_VARIABLE _head OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
  if(NOT _head STREQUAL _commit)
    message(FATAL_ERROR "${_git} gave ${_head} for ${_commit}")
  endif()
  file(REMOVE_RECURSE "${_staging}/.git")
  file(RENAME "${_staging}" "${dir}")
endfunction()

set(RN_WEBOS_SKIA_SOURCE_DIR "${RN_WEBOS_SKIA_ROOT}/src/skia-${_skia_pins_id}")
_rn_webos_skia_fetch(skia "${RN_WEBOS_SKIA_SOURCE_DIR}")

# Skia's build reads its dependencies at third_party/externals/<name>, and no
# GN argument moves them.
string(JSON _skia_pin_count LENGTH "${_skia_pins}")
math(EXPR _skia_pin_last "${_skia_pin_count} - 1")
foreach(_index RANGE ${_skia_pin_last})
  string(JSON _name MEMBER "${_skia_pins}" ${_index})
  if(_name STREQUAL "skia")
    continue()
  endif()
  string(JSON _pin GET "${_skia_pins}" ${_name})
  string(SHA256 _pin_id "${_pin}")
  string(SUBSTRING "${_pin_id}" 0 12 _pin_id)
  set(_dir "${RN_WEBOS_SKIA_ROOT}/src/${_name}-${_pin_id}")
  _rn_webos_skia_fetch(${_name} "${_dir}")
  set(_link "${RN_WEBOS_SKIA_SOURCE_DIR}/third_party/externals/${_name}")
  if(NOT IS_SYMLINK "${_link}")
    file(MAKE_DIRECTORY "${RN_WEBOS_SKIA_SOURCE_DIR}/third_party/externals")
    file(CREATE_LINK "${_dir}" "${_link}" SYMBOLIC)
  endif()
endforeach()

# SkUnicode's ICU backend compiles against the headers of the ICU Icu.cmake
# builds, so its calls carry that ICU's renamed symbols. The copy sits beside
# the build that includes it: a runtime build directory may be deleted.
string(JSON _skia_icu_version GET "${_rn_webos_dependencies}" icu version)
set(_skia_icu_include "${RN_WEBOS_SKIA_ROOT}/icu-${_skia_icu_version}/include")
if(NOT EXISTS "${_skia_icu_include}/unicode/uvernum.h")
  file(COPY "${CMAKE_BINARY_DIR}/icu/include/unicode" DESTINATION "${_skia_icu_include}")
endif()

set(_skia_cflags
  ${RN_WEBOS_SKIA_CFLAGS}
  "-I${_skia_icu_include}" -DU_STATIC_IMPLEMENTATION -DU_CHARSET_IS_UTF8=1 -DU_HIDE_OBSOLETE_UTF_OLD_H=1
  -DMESA_EGL_NO_X11_HEADERS -fPIC)
list(TRANSFORM _skia_cflags PREPEND "\"")
list(TRANSFORM _skia_cflags APPEND "\"")
list(JOIN _skia_cflags ", " _skia_cflags)

set(_skia_args
  "is_official_build = true"
  "is_component_build = false"
  "skia_use_partition_alloc = false"
  "skia_enable_tools = false"
  "skia_enable_ganesh = true"
  "skia_enable_graphite = false"
  "skia_use_gl = true"
  "skia_use_egl = true"
  "skia_gl_standard = \"gles\""
  "skia_use_x11 = false"
  "skia_use_vulkan = false"
  "skia_use_freetype = true"
  "skia_use_system_freetype2 = false"
  "skia_use_fontconfig = false"
  "skia_enable_fontmgr_custom_directory = true"
  "skia_enable_fontmgr_custom_empty = true"
  "skia_enable_fontmgr_custom_embedded = false"
  "skia_enable_fontmgr_android = false"
  "skia_use_harfbuzz = true"
  "skia_use_system_harfbuzz = false"
  "skia_pdf_subset_harfbuzz = false"
  "skia_enable_skshaper = true"
  "skia_enable_skparagraph = true"
  "skia_use_icu = true"
  "skia_use_system_icu = true"
  "skia_use_libpng_decode = true"
  "skia_use_libpng_encode = true"
  "skia_use_system_libpng = false"
  "skia_use_libjpeg_turbo_decode = true"
  "skia_use_libjpeg_turbo_encode = false"
  "skia_use_no_jpeg_encode = true"
  "skia_use_system_libjpeg_turbo = false"
  "skia_use_libwebp_decode = true"
  "skia_use_libwebp_encode = false"
  "skia_use_no_webp_encode = true"
  "skia_use_system_libwebp = false"
  "skia_use_wuffs = true"
  "skia_use_system_zlib = false"
  "skia_use_expat = true"
  "skia_use_system_expat = false"
  "skia_enable_svg = true"
  "skia_enable_pdf = false"
  "skia_enable_skottie = false"
  "skia_use_dng_sdk = false"
  "skia_use_piex = false"
  "skia_use_perfetto = false"
  # The host compiles with RTTI and derives from Skia's classes.
  "extra_cflags_cc = [ \"-frtti\" ]"
  "extra_cflags = [ ${_skia_cflags} ]"
  "target_os = \"linux\"")
if(RN_WEBOS_SKIA_TARGET_ARGS)
  list(APPEND _skia_args ${RN_WEBOS_SKIA_TARGET_ARGS})
else()
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(_skia_cpu arm64)
  else()
    set(_skia_cpu x64)
  endif()
  list(APPEND _skia_args
    "target_cpu = \"${_skia_cpu}\""
    "cc = \"${CMAKE_C_COMPILER}\""
    "cxx = \"${CMAKE_CXX_COMPILER}\"")
endif()
if(RN_WEBOS_CCACHE)
  list(APPEND _skia_args "cc_wrapper = \"${RN_WEBOS_CCACHE}\"")
endif()
list(JOIN _skia_args "\n" _skia_args_text)

string(SHA256 _skia_key "${_skia_pins}\n${_skia_args_text}\n${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
string(SUBSTRING "${_skia_key}" 0 12 _skia_key)
set(RN_WEBOS_SKIA_OUT_DIR "${RN_WEBOS_SKIA_ROOT}/out/${_skia_key}")

if(NOT EXISTS "${RN_WEBOS_SKIA_OUT_DIR}/build.ninja")
  file(MAKE_DIRECTORY "${RN_WEBOS_SKIA_OUT_DIR}")
  file(WRITE "${RN_WEBOS_SKIA_OUT_DIR}/args.gn" "${_skia_args_text}\n")
  execute_process(
    COMMAND "${RN_WEBOS_GN}" gen "${RN_WEBOS_SKIA_OUT_DIR}" "--script-executable=${Python3_EXECUTABLE}"
    WORKING_DIRECTORY "${RN_WEBOS_SKIA_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY)
endif()

# Skia's public headers change with the defines its own sources were compiled
# with (SK_GANESH, SK_GL, SK_RELEASE...): GN wrote them to the target's file.
file(STRINGS "${RN_WEBOS_SKIA_OUT_DIR}/obj/skia.ninja" _skia_defines REGEX "^defines = ")
string(REGEX REPLACE "^defines = " "" _skia_defines "${_skia_defines}")
separate_arguments(_skia_defines UNIX_COMMAND "${_skia_defines}")
list(FILTER _skia_defines INCLUDE REGEX "^-DSK_")
list(TRANSFORM _skia_defines REPLACE "^-D" "")

set(_skia_modules skparagraph skshaper skunicode_icu skunicode_core svg skresources skia)
set(_skia_archives "")
foreach(_module IN LISTS _skia_modules)
  list(APPEND _skia_archives "${RN_WEBOS_SKIA_OUT_DIR}/lib${_module}.a")
endforeach()
list(JOIN _skia_modules " " _skia_ninja_targets)

add_custom_target(rn_webos_skia_build
  COMMAND "${CMAKE_COMMAND}"
    "-DOUT=${RN_WEBOS_SKIA_OUT_DIR}" "-DNINJA=${RN_WEBOS_NINJA}" "-DTARGETS=${_skia_ninja_targets}"
    -P "${CMAKE_CURRENT_LIST_DIR}/../skia/build.cmake"
  BYPRODUCTS ${_skia_archives}
  USES_TERMINAL
  COMMENT "Building Skia in ${RN_WEBOS_SKIA_OUT_DIR}"
  VERBATIM)

add_library(rn_webos_skia INTERFACE)
add_dependencies(rn_webos_skia rn_webos_skia_build)
target_include_directories(rn_webos_skia SYSTEM INTERFACE "${RN_WEBOS_SKIA_SOURCE_DIR}")
target_compile_definitions(rn_webos_skia INTERFACE ${_skia_defines} SK_RELEASE U_USING_ICU_NAMESPACE=0)
target_link_libraries(rn_webos_skia INTERFACE ${_skia_archives} rn_webos_icuuc ${CMAKE_DL_LIBS} Threads::Threads)
