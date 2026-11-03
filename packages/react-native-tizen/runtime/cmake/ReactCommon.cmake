# React Native's C++ core, compiled from the app's own react-native package with
# the CMakeLists it ships for Android. Off Android those select the portable
# `cxx` platform sources and drop their JNI dependencies.

set(REACT_COMMON_DIR "${REACT_NATIVE_DIR}/ReactCommon")
set(REACT_ANDROID_DIR "${REACT_NATIVE_DIR}/ReactAndroid")

set(RN_TIZEN_REACT_COMMON_DIRS
  yoga runtimeexecutor reactperflogger logger jsiexecutor jsitooling cxxreact callinvoker oscompat
  jsinspector-modern jsinspector-modern/cdp jsinspector-modern/network jsinspector-modern/tracing
  hermes/executor hermes/inspector-modern
  react/cxxstableapi react/debug react/featureflags react/timing react/utils react/bridging react/networking
  react/performance/cdpmetrics react/performance/timeline
  react/renderer/runtimescheduler react/renderer/animated react/renderer/animationbackend
  react/renderer/animations react/renderer/attributedstring react/renderer/componentregistry
  react/renderer/componentregistry/native react/renderer/mounting react/renderer/scheduler
  react/renderer/telemetry react/renderer/viewtransition react/renderer/uimanager
  react/renderer/uimanager/consistency react/renderer/bridging react/renderer/core
  react/renderer/consistency react/renderer/css react/renderer/dom react/renderer/element
  react/renderer/debug react/renderer/graphics react/renderer/imagemanager react/renderer/mapbuffer
  react/renderer/leakchecker react/renderer/observers/events react/renderer/observers/intersection
  react/renderer/observers/mutation react/renderer/observers/resize
  react/renderer/components/view react/renderer/components/root react/renderer/components/image
  react/renderer/components/text react/renderer/components/textinput
  react/renderer/components/scrollview react/renderer/components/modal
  react/renderer/components/legacyviewmanagerinterop react/renderer/components/unimplementedview
  react/nativemodule/core react/nativemodule/defaults react/nativemodule/dom
  react/nativemodule/featureflags react/nativemodule/microtasks react/nativemodule/idlecallbacks
  react/nativemodule/intersectionobserver react/nativemodule/mutationobserver
  react/nativemodule/resizeobserver react/nativemodule/viewtransition react/nativemodule/webperformance
  react/nativemodule/devtoolsruntimesettings devtoolsruntimesettings
  jserrorhandler react/runtime react/runtime/hermes react/runtime/nativeviewconfig)

# React Native's sources lean on libc++ including <cstdint> transitively, which
# libstdc++ does not.
set(_saved_cxx_flags "${CMAKE_CXX_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS " -include cstdint")
foreach(_dir IN LISTS RN_TIZEN_REACT_COMMON_DIRS)
  add_subdirectory("${REACT_COMMON_DIR}/${_dir}" "ReactCommon/${_dir}" EXCLUDE_FROM_ALL)
endforeach()
set(CMAKE_CXX_FLAGS "${_saved_cxx_flags}")

# ReactCommon links `yoga`, which only ReactAndroid's JNI layer defines.
add_library(yoga INTERFACE)
target_link_libraries(yoga INTERFACE yogacore)

# React Native builds with -Werror against the NDK's clang. A newer clang warns
# about code React Native has not seen warnings for yet, which must not stop a build.
function(rn_tizen_relax_warnings directory)
  get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_target IN LISTS _targets)
    get_target_property(_type ${_target} TYPE)
    set(_properties INTERFACE_COMPILE_OPTIONS)
    if(NOT _type STREQUAL "INTERFACE_LIBRARY")
      list(APPEND _properties COMPILE_OPTIONS)
    endif()
    foreach(_property IN LISTS _properties)
      get_target_property(_options ${_target} ${_property})
      if(_options)
        list(REMOVE_ITEM _options -Werror -Wpedantic)
        set_property(TARGET ${_target} PROPERTY ${_property} ${_options})
      endif()
    endforeach()
  endforeach()
  get_property(_children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(_child IN LISTS _children)
    rn_tizen_relax_warnings("${_child}")
  endforeach()
endfunction()

foreach(_dir IN LISTS RN_TIZEN_REACT_COMMON_DIRS)
  rn_tizen_relax_warnings("${REACT_COMMON_DIR}/${_dir}")
endforeach()
