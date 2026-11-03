# The app's native libraries: every package with React Native codegen specs, as
# `rn-tizen` lists them in RN_TIZEN_LIBRARIES_FILE (docs/native-modules.md). Each
# becomes an OBJECT library linked into libreact-native-tizen.so, so its
# self-registering static initializers are kept: its codegen output, plus the
# sources its Tizen implementation's CMakeLists.txt adds to
# ${RN_TIZEN_LIBRARY_TARGET}.

set(RN_TIZEN_LIBRARIES_FILE "" CACHE FILEPATH "The native libraries rn-tizen found in the app (libraries.json)")

set(_rn_tizen_host_src "${CMAKE_CURRENT_SOURCE_DIR}/host/src")

# An implementation bundled under host/src (a directory with a library.json)
# compiles only for an app that depends on its package, through its own
# CMakeLists.txt, never through the host's glob.
file(GLOB _bundled_manifests CONFIGURE_DEPENDS "${_rn_tizen_host_src}/*/library.json")
get_target_property(_host_sources rn_tizen_host SOURCES)
set(_kept_sources "")
foreach(_source IN LISTS _host_sources)
  set(_bundled FALSE)
  foreach(_manifest IN LISTS _bundled_manifests)
    get_filename_component(_bundled_dir "${_manifest}" DIRECTORY)
    string(FIND "${_source}" "${_bundled_dir}/" _at)
    if(_at EQUAL 0)
      set(_bundled TRUE)
    endif()
  endforeach()
  if(NOT _bundled)
    list(APPEND _kept_sources "${_source}")
  endif()
endforeach()
if(NOT _kept_sources STREQUAL _host_sources)
  set_property(TARGET rn_tizen_host PROPERTY SOURCES ${_kept_sources})
endif()

function(rn_tizen_add_native_library package package_dir codegen_name codegen_dir implementation)
  set(_target "rn_tizen_library_${codegen_name}")
  file(GLOB _generated CONFIGURE_DEPENDS
    "${codegen_dir}/*.cpp"
    "${codegen_dir}/react/renderer/components/${codegen_name}/*.cpp")
  add_library(${_target} OBJECT ${_generated})
  target_include_directories(${_target}
    PUBLIC "${codegen_dir}"
    PRIVATE "${_rn_tizen_host_src}")
  target_link_libraries(${_target} PUBLIC
    folly_runtime glog jsi react_codegen_rncore react_debug react_nativemodule_core
    react_renderer_componentregistry react_renderer_core react_renderer_debug react_renderer_graphics
    react_renderer_imagemanager react_renderer_mapbuffer react_utils rrc_image rrc_view yoga)
  target_compile_reactnative_options(${_target} PRIVATE)
  target_compile_options(${_target} PRIVATE -Wno-error -include cstdint)
  if(TARGET expo_modules_common)
    target_include_directories(${_target} PRIVATE
      "${_rn_tizen_host_src}/.." $<TARGET_PROPERTY:expo_modules_common,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(${_target} PRIVATE
      $<TARGET_PROPERTY:expo_modules_common,INTERFACE_COMPILE_DEFINITIONS>)
  endif()

  if(implementation)
    if(NOT EXISTS "${implementation}")
      message(FATAL_ERROR "${package} declares a Tizen implementation at ${implementation}, which does not exist")
    endif()
    set(RN_TIZEN_LIBRARY_TARGET ${_target})
    set(RN_TIZEN_LIBRARY_PACKAGE "${package}")
    set(RN_TIZEN_LIBRARY_DIR "${package_dir}")
    get_filename_component(_implementation_dir "${implementation}" DIRECTORY)
    add_subdirectory("${_implementation_dir}" "${CMAKE_BINARY_DIR}/libraries/${codegen_name}")
  endif()

  target_link_libraries(react-native-tizen PRIVATE ${_target})
endfunction()

if(NOT RN_TIZEN_LIBRARIES_FILE)
  return()
endif()
if(NOT EXISTS "${RN_TIZEN_LIBRARIES_FILE}")
  message(FATAL_ERROR "RN_TIZEN_LIBRARIES_FILE (${RN_TIZEN_LIBRARIES_FILE}) does not exist")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RN_TIZEN_LIBRARIES_FILE}")

file(READ "${RN_TIZEN_LIBRARIES_FILE}" _libraries_json)
string(JSON _library_count LENGTH "${_libraries_json}" libraries)
if(_library_count EQUAL 0)
  return()
endif()
math(EXPR _last_library "${_library_count} - 1")
foreach(_index RANGE ${_last_library})
  foreach(_field name dir codegenName codegenDir implementation)
    string(JSON _library_${_field} GET "${_libraries_json}" libraries ${_index} ${_field})
  endforeach()
  rn_tizen_add_native_library(
    "${_library_name}" "${_library_dir}" "${_library_codegenName}" "${_library_codegenDir}"
    "${_library_implementation}")
  message(STATUS "Native library ${_library_name}: codegen ${_library_codegenName}")
endforeach()
