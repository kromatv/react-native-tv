# The ReactCommon pieces whose implementation is per platform, built from the
# shared sources plus the Tizen implementation in this directory.

set(_text_dir "${REACT_COMMON_DIR}/react/renderer/textlayoutmanager")
file(GLOB _text_sources CONFIGURE_DEPENDS "${_text_dir}/*.cpp")
add_library(react_renderer_textlayoutmanager OBJECT
  ${_text_sources}
  react/renderer/textlayoutmanager/TextLayoutManager.cpp)
target_include_directories(react_renderer_textlayoutmanager PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${_text_dir}"
  "${REACT_COMMON_DIR}")
target_link_libraries(react_renderer_textlayoutmanager PUBLIC
  folly_runtime glog react_debug react_renderer_attributedstring react_renderer_componentregistry
  react_renderer_core react_renderer_debug react_renderer_graphics react_renderer_mapbuffer
  react_renderer_mounting react_renderer_telemetry react_utils yoga)
target_compile_reactnative_options(react_renderer_textlayoutmanager PRIVATE)
