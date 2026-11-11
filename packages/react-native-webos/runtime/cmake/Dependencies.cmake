# The C++ libraries React Native and Hermes build against, pinned to the
# versions react-native 0.88 itself uses (third-party-podspecs, ReactAndroid).
# The pins live in dependencies.json, which `rn-webos` reads too. Archives land
# in RN_WEBOS_DOWNLOAD_DIR, shared by every build on the machine.

include(FetchContent)

set(RN_WEBOS_DOWNLOAD_DIR "${RN_WEBOS_CACHE_DIR}/downloads" CACHE PATH
  "Where dependency archives are downloaded and kept")

set(_rn_webos_dependencies_file "${CMAKE_CURRENT_LIST_DIR}/../dependencies.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_rn_webos_dependencies_file}")
file(READ "${_rn_webos_dependencies_file}" _rn_webos_dependencies)

string(JSON _count LENGTH "${_rn_webos_dependencies}")
math(EXPR _last "${_count} - 1")
set(_rn_webos_dependency_names "")
foreach(_index RANGE ${_last})
  string(JSON _name MEMBER "${_rn_webos_dependencies}" ${_index})
  string(JSON _url GET "${_rn_webos_dependencies}" ${_name} url)
  string(JSON _sha256 GET "${_rn_webos_dependencies}" ${_name} sha256)
  get_filename_component(_archive "${_url}" NAME)
  FetchContent_Declare(${_name}
    URL "${_url}"
    URL_HASH SHA256=${_sha256}
    DOWNLOAD_DIR "${RN_WEBOS_DOWNLOAD_DIR}/${_name}"
    DOWNLOAD_NAME "${_name}-${_archive}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR "rn-webos-no-cmakelists")
  list(APPEND _rn_webos_dependency_names ${_name})
endforeach()

FetchContent_MakeAvailable(${_rn_webos_dependency_names})
