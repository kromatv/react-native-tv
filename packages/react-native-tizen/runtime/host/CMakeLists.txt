# libreact-native-tizen.so: Hermes, React Native's C++ core and the Tizen host,
# linked into the one library a Tizen app loads. It exports the C API in
# include/react-native-tizen/react-native-tizen.h and nothing else.

file(GLOB_RECURSE _host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(TARGET expo_modules_common)
  file(GLOB_RECURSE _expo_host_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/expo/*.cpp")
  list(APPEND _host_sources ${_expo_host_sources})
endif()
# React Native's cross-platform TextInput props. Its Android props, shadow node
# and state need Android's serializable props or Java, so host/src/fabric has
# its own.
set(_textinput_dir "${REACT_COMMON_DIR}/react/renderer/components/textinput")
set(_textinput_sources "${_textinput_dir}/BaseTextInputProps.cpp")
set_source_files_properties(${_textinput_sources} PROPERTIES COMPILE_OPTIONS "-include;cstdint")
list(APPEND _host_sources ${_textinput_sources})
add_library(react-native-tizen SHARED ${_host_sources})

target_include_directories(react-native-tizen
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_reactnative_options(react-native-tizen PRIVATE)
target_compile_options(react-native-tizen PRIVATE -Wno-error)
set_target_properties(react-native-tizen PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)

set(_react_native_roots
  bridgeless bridgelesshermes bridgelessnativeviewconfig
  react_renderer_scheduler react_renderer_uimanager react_renderer_mounting react_renderer_componentregistry
  rrc_root rrc_view rrc_text rrc_image rrc_scrollview rrc_modal rrc_native react_codegen_rncore
  react_renderer_textlayoutmanager react_nativemodule_core react_nativemodule_defaults
  jsi jsireact jsitooling hermesvm_a folly_runtime glog)
if(TARGET expo_modules_common)
  list(APPEND _react_native_roots expo_modules_common)
  target_compile_definitions(react-native-tizen PRIVATE RN_TIZEN_EXPO=1)
endif()
target_link_libraries(react-native-tizen PRIVATE ${_react_native_roots} Threads::Threads ${CMAKE_DL_LIBS})
target_link_tizen_libraries(react-native-tizen
  elementary ecore ecore_evas ecore_imf ecore_input evas eina eo efl-extension dlog curl capi-system-info
  capi-appfw-application capi-appfw-app-common capi-media-player)
rn_tizen_link_object_closure(react-native-tizen ${_react_native_roots})
target_link_options(react-native-tizen PRIVATE
  "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
  "-Wl,--gc-sections")
set_property(TARGET react-native-tizen APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
