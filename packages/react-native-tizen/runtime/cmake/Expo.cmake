# Expo Modules on Tizen: the platform-independent C++ of the app's own
# expo-modules-core (the EventEmitter, NativeModule, SharedObject and SharedRef
# classes behind `globalThis.expo`, and the Fabric pieces every Expo view is
# described with). Skipped for a React Native app without Expo.

set(EXPO_MODULES_CORE_DIR "" CACHE PATH "The app's expo-modules-core package; empty for an app without Expo")
set(EXPO_MODULES_JSI_DIR "" CACHE PATH "The expo-modules-jsi package expo-modules-core depends on")

if(NOT EXPO_MODULES_CORE_DIR)
  return()
endif()
foreach(_expo_dir EXPO_MODULES_CORE_DIR EXPO_MODULES_JSI_DIR)
  if(NOT EXISTS "${${_expo_dir}}/package.json")
    message(FATAL_ERROR "${_expo_dir} must be an npm package, got '${${_expo_dir}}'")
  endif()
endforeach()

file(READ "${EXPO_MODULES_CORE_DIR}/package.json" _expo_package)
string(JSON EXPO_MODULES_CORE_VERSION GET "${_expo_package}" version)

set(_expo_common "${EXPO_MODULES_CORE_DIR}/common/cpp")
file(GLOB _expo_sources CONFIGURE_DEPENDS
  "${_expo_common}/*.cpp" "${_expo_common}/JSI/*.cpp" "${_expo_common}/fabric/*.cpp")
add_library(expo_modules_common OBJECT ${_expo_sources})
target_include_directories(expo_modules_common PUBLIC "${_expo_common}" "${_expo_common}/JSI" "${_expo_common}/fabric")
target_compile_definitions(expo_modules_common PUBLIC
  IS_NEW_ARCHITECTURE_ENABLED=1 RN_FABRIC_ENABLED=1 USE_HERMES=1 UNIT_TEST=0
  EXPO_MODULES_CORE_VERSION="${EXPO_MODULES_CORE_VERSION}")
target_link_libraries(expo_modules_common PUBLIC
  jsi react_cxxreact react_nativemodule_core react_bridging folly_runtime rrc_view react_renderer_core)
target_compile_options(expo_modules_common PRIVATE -frtti -fexceptions -include cstdint)
