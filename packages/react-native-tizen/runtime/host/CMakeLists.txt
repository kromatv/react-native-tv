# libreact-native-tizen.so: Hermes, React Native's C++ core and the Tizen host,
# linked into the one library a Tizen app loads. It exports the C API in
# include/react-native-tizen/react-native-tizen.h and nothing else.

file(GLOB_RECURSE _host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER _host_sources EXCLUDE REGEX "\\.test\\.cpp$")
if(TARGET expo_modules_common)
  file(GLOB_RECURSE _expo_host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/expo/*.cpp")
  list(APPEND _host_sources ${_expo_host_sources})
endif()
if(RN_TIZEN_DEVELOPMENT)
  file(GLOB_RECURSE _devsupport_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/devsupport/*.cpp")
  list(FILTER _devsupport_sources EXCLUDE REGEX "\\.test\\.cpp$")
  list(APPEND _host_sources ${_devsupport_sources})
endif()
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
set(_tizen_libraries
  elementary ecore ecore_evas ecore_imf ecore_input evas eina eo efl-extension dlog curl capi-system-info
  capi-appfw-application capi-appfw-app-common capi-media-player)

# The host's objects, which the library links as they are and the tests in
# src/**/*.test.cpp link from an archive, so a test pulls in only what it uses.
add_library(rn_tizen_host OBJECT ${_host_sources})
target_include_directories(rn_tizen_host
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_reactnative_options(rn_tizen_host PRIVATE)
target_compile_options(rn_tizen_host PRIVATE -Wno-error)
set_target_properties(rn_tizen_host PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)
if(TARGET expo_modules_common)
  list(APPEND _react_native_roots expo_modules_common)
  target_compile_definitions(rn_tizen_host PRIVATE RN_TIZEN_EXPO=1)
endif()
if(RN_TIZEN_DEVELOPMENT)
  target_compile_definitions(rn_tizen_host PRIVATE RN_TIZEN_DEV_SUPPORT=1)
endif()
target_link_libraries(rn_tizen_host PUBLIC ${_react_native_roots} Threads::Threads ${CMAKE_DL_LIBS})
target_link_tizen_libraries(rn_tizen_host ${_tizen_libraries})

add_library(react-native-tizen SHARED)
target_include_directories(react-native-tizen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(react-native-tizen PRIVATE rn_tizen_host)
target_link_tizen_libraries(react-native-tizen ${_tizen_libraries})
rn_tizen_link_object_closure(react-native-tizen ${_react_native_roots})
target_link_options(react-native-tizen PRIVATE
  "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
  "-Wl,--gc-sections")
set_property(TARGET react-native-tizen APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")

if(RN_TIZEN_TESTS)
  file(GLOB_RECURSE _host_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.test.cpp")
  if(RN_TIZEN_DEVELOPMENT)
    file(GLOB_RECURSE _devsupport_test_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/devsupport/*.test.cpp")
    list(APPEND _host_test_sources ${_devsupport_test_sources})
  endif()
  rn_tizen_add_host_tests(rn-tizen-host-tests
    SOURCES ${_host_test_sources}
    HOST rn_tizen_host
    ROOTS ${_react_native_roots}
    TIZEN_LIBRARIES ${_tizen_libraries}
    INSTRUMENT react_renderer_textlayoutmanager hermesPlatformIntl_obj)
  # react-native-svg's implementation compiles only for an app that uses it
  # (cmake/Libraries.cmake); its path and viewBox math needs no package.
  target_sources(rn-tizen-host-tests PRIVATE
    src/svg/PathData.cpp src/svg/SvgPath.cpp src/svg/SvgViewBox.cpp)
endif()
