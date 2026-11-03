# The C++ React Native's codegen produces for its own specs, under the target
# name ReactCommon links: `rn-tizen` generates it before configuring.

set(RN_TIZEN_CODEGEN_DIR "" CACHE PATH "Output of rn-tizen's React Native codegen")
set(_codegen_components "${RN_TIZEN_CODEGEN_DIR}/react/renderer/components/FBReactNativeSpec")
if(NOT EXISTS "${_codegen_components}/Props.h" OR NOT EXISTS "${RN_TIZEN_CODEGEN_DIR}/FBReactNativeSpec/FBReactNativeSpecJSI.h")
  message(FATAL_ERROR "RN_TIZEN_CODEGEN_DIR (${RN_TIZEN_CODEGEN_DIR}) holds no React Native codegen output.")
endif()

file(GLOB _codegen_sources CONFIGURE_DEPENDS "${_codegen_components}/*.cpp")
add_library(react_codegen_rncore OBJECT ${_codegen_sources})
target_include_directories(react_codegen_rncore PUBLIC "${RN_TIZEN_CODEGEN_DIR}")
target_link_libraries(react_codegen_rncore PUBLIC
  folly_runtime glog jsi react_debug react_nativemodule_core react_renderer_componentregistry
  react_renderer_core react_renderer_debug react_renderer_graphics react_renderer_imagemanager
  react_renderer_mapbuffer react_utils rrc_image rrc_view yoga)
target_compile_reactnative_options(react_codegen_rncore PRIVATE)
