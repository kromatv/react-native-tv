# react-native-svg on webOS: the package's own C++ shadow nodes beside its
# codegen output, and the views that paint them with Evas VG.

file(GLOB _svg_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${RN_WEBOS_LIBRARY_DIR}/common/cpp/react/renderer/components/rnsvg/*.cpp")
list(FILTER _svg_sources EXCLUDE REGEX "\\.test\\.cpp$")
target_sources(${RN_WEBOS_LIBRARY_TARGET} PRIVATE ${_svg_sources})
target_include_directories(${RN_WEBOS_LIBRARY_TARGET} PUBLIC "${RN_WEBOS_LIBRARY_DIR}/common/cpp")
target_link_libraries(${RN_WEBOS_LIBRARY_TARGET} PUBLIC rrc_image react_renderer_imagemanager)
target_link_webos_libraries(${RN_WEBOS_LIBRARY_TARGET} evas ecore eina eo)

set(_svg_evas_legacy "${CMAKE_SYSROOT}/usr/include/evas-1/Evas_Legacy.h")
if(EXISTS "${_svg_evas_legacy}")
  file(STRINGS "${_svg_evas_legacy}" _svg_vg_undeclared REGEX "^//EAPI void evas_vg_shape_reset\\(")
  if(_svg_vg_undeclared)
    target_compile_definitions(${RN_WEBOS_LIBRARY_TARGET} PRIVATE RN_WEBOS_DECLARE_EVAS_VG=1)
  endif()
endif()
