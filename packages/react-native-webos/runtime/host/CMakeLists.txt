# react-native-webos: Hermes, React Native's C++ core and the webOS host, linked
# into the one executable an app's package starts.

file(GLOB_RECURSE _host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER _host_sources EXCLUDE REGEX "\\.test\\.cpp$")
if(TARGET expo_modules_common)
  file(GLOB_RECURSE _expo_host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/expo/*.cpp")
  list(FILTER _expo_host_sources EXCLUDE REGEX "\\.test\\.cpp$")
  list(APPEND _host_sources ${_expo_host_sources})
endif()
if(RN_WEBOS_DEVELOPMENT)
  file(GLOB_RECURSE _devsupport_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/devsupport/*.cpp")
  list(FILTER _devsupport_sources EXCLUDE REGEX "\\.test\\.cpp$")
  list(APPEND _host_sources ${_devsupport_sources})
endif()
list(FILTER _host_sources EXCLUDE REGEX "/src/main\\.cpp$")
foreach(_unported IN LISTS RN_WEBOS_UNPORTED)
  list(FILTER _host_sources EXCLUDE REGEX "${_unported}")
endforeach()
# React Native's cross-platform TextInput props. Its Android props, shadow node
# and state need Android's serializable props or Java, so host/src/fabric has
# its own.
set(_textinput_dir "${REACT_COMMON_DIR}/react/renderer/components/textinput")
set(_textinput_sources "${_textinput_dir}/BaseTextInputProps.cpp")
set_source_files_properties(${_textinput_sources} PROPERTIES COMPILE_OPTIONS "-include;cstdint")
list(APPEND _host_sources ${_textinput_sources})

set(_react_native_roots
  bridgeless bridgelesshermes bridgelessnativeviewconfig
  react_renderer_scheduler react_renderer_uimanager react_renderer_mounting react_renderer_componentregistry
  rrc_root rrc_view rrc_text rrc_image rrc_scrollview rrc_modal rrc_native react_codegen_rncore
  react_renderer_textlayoutmanager react_nativemodule_core react_nativemodule_defaults
  jsi jsireact jsitooling hermesvm_a folly_runtime glog)

# The host's objects, which the executable links as they are and the tests in
# src/**/*.test.cpp link from an archive, so a test pulls in only what it uses.
add_library(rn_webos_host OBJECT ${_host_sources})
target_include_directories(rn_webos_host PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_reactnative_options(rn_webos_host PRIVATE)
target_compile_options(rn_webos_host PRIVATE -Wno-error)
if(TARGET expo_modules_common)
  list(APPEND _react_native_roots expo_modules_common)
  target_compile_definitions(rn_webos_host PRIVATE RN_WEBOS_EXPO=1)
endif()
if(RN_WEBOS_DEVELOPMENT AND NOT RN_WEBOS_UNPORTED)
  target_compile_definitions(rn_webos_host PRIVATE RN_WEBOS_DEV_SUPPORT=1)
endif()
if(RN_WEBOS_SIMULATOR)
  target_compile_definitions(rn_webos_host PUBLIC RN_WEBOS_SIMULATOR=1)
endif()
target_link_libraries(rn_webos_host PUBLIC
  ${_react_native_roots} rn_webos_skia SDL3::SDL3-static CURL::libcurl rn_webos_platform Threads::Threads
  ${CMAKE_DL_LIBS})

add_executable(react-native-webos src/main.cpp)
target_compile_reactnative_options(react-native-webos PRIVATE)
target_compile_options(react-native-webos PRIVATE -Wno-error)
target_link_libraries(react-native-webos PRIVATE rn_webos_host)
rn_webos_link_object_closure(react-native-webos ${_react_native_roots})
target_link_options(react-native-webos PRIVATE "-Wl,--gc-sections")

if(RN_WEBOS_TESTS)
  file(GLOB_RECURSE _host_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.test.cpp")
  if(TARGET expo_modules_common)
    file(GLOB_RECURSE _expo_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/expo/*.test.cpp")
    list(APPEND _host_test_sources ${_expo_test_sources})
  endif()
  if(RN_WEBOS_DEVELOPMENT)
    file(GLOB_RECURSE _devsupport_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/devsupport/*.test.cpp")
    list(APPEND _host_test_sources ${_devsupport_test_sources})
  endif()
  foreach(_unported IN LISTS RN_WEBOS_UNPORTED RN_WEBOS_UNPORTED_TESTS)
    list(FILTER _host_test_sources EXCLUDE REGEX "${_unported}")
  endforeach()
  rn_webos_add_host_tests(rn-webos-host-tests
    SOURCES ${_host_test_sources}
    HOST rn_webos_host
    ROOTS ${_react_native_roots}
    INSTRUMENT react_renderer_textlayoutmanager hermesPlatformIntl_obj)
  if(TARGET expo_modules_common)
    # An Expo module registers itself from a static initializer in its own file
    # (RN_WEBOS_EXPO_MODULE), which never runs from an archive member no test
    # references: link the Expo host's objects whole, as the executable does.
    target_sources(rn-webos-host-tests PRIVATE "$<FILTER:$<TARGET_OBJECTS:rn_webos_host>,INCLUDE,/expo/>")
  endif()
  # A component descriptor registers itself from a static initializer too
  # (RN_WEBOS_COMPONENT_DESCRIPTOR): link the core's, so the registry a test
  # builds has them.
  target_sources(rn-webos-host-tests PRIVATE
    "$<FILTER:$<TARGET_OBJECTS:rn_webos_host>,INCLUDE,/fabric/[A-Za-z]+ComponentDescriptors?\\.cpp\\.o>")
  if(NOT "/svg/" IN_LIST RN_WEBOS_UNPORTED)
    # react-native-svg's implementation compiles only for an app that uses it
    # (cmake/Libraries.cmake); its path and viewBox math needs no package.
    target_sources(rn-webos-host-tests PRIVATE
      src/svg/PathData.cpp src/svg/SvgPath.cpp src/svg/SvgViewBox.cpp)
  endif()
  # The full ICU data file ICU's sources ship, for the host's ICU setup to load.
  set_source_files_properties(src/intl/IcuData.test.cpp PROPERTIES
    COMPILE_DEFINITIONS "RN_WEBOS_TEST_ICU_DATA_DIR=\"${icu_SOURCE_DIR}/source/data/in\"")
endif()
