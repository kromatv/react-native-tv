# react-native-svg on webOS: the package's own C++ shadow nodes beside its
# codegen output, and the views that draw its elements with Skia.

file(GLOB _svg_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${RN_WEBOS_LIBRARY_DIR}/common/cpp/react/renderer/components/rnsvg/*.cpp")
list(FILTER _svg_sources EXCLUDE REGEX "\\.test\\.cpp$")
target_sources(${RN_WEBOS_LIBRARY_TARGET} PRIVATE ${_svg_sources})
target_include_directories(${RN_WEBOS_LIBRARY_TARGET} PUBLIC "${RN_WEBOS_LIBRARY_DIR}/common/cpp")
# A view includes views/ComponentView.h, which needs the headers of what the
# host draws and opens its window with.
target_link_libraries(${RN_WEBOS_LIBRARY_TARGET} PUBLIC
  rrc_image react_renderer_imagemanager rn_webos_skia SDL3::SDL3-static)
