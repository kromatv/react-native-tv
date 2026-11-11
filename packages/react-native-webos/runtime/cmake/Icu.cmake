# ICU, built from source and linked statically, for Hermes' Unicode support
# and its Intl. Its data is not linked: each app ships an icudt<version>l.dat
# trimmed to its locales, which the host points ICU at (docs/decisions.md).

set(_icu_source "${icu_SOURCE_DIR}/source")
foreach(_part common i18n stubdata)
  file(STRINGS "${_icu_source}/${_part}/sources.txt" _icu_${_part}_sources)
  list(TRANSFORM _icu_${_part}_sources PREPEND "${_icu_source}/${_part}/")
endforeach()

# ICU's source directories hold internal headers too, such as sharedobject.h,
# which a case-insensitive file system hands to any `#include <SharedObject.h>`:
# users see the public headers only, as an installed ICU lays them out.
set(_icu_include "${CMAKE_BINARY_DIR}/icu/include")
file(GLOB _icu_public_headers "${_icu_source}/common/unicode/*.h" "${_icu_source}/i18n/unicode/*.h")
file(COPY ${_icu_public_headers} DESTINATION "${_icu_include}/unicode")

add_library(rn_tizen_icuuc STATIC ${_icu_common_sources} ${_icu_stubdata_sources})
target_include_directories(rn_tizen_icuuc
  PUBLIC "${_icu_include}"
  PRIVATE "${_icu_source}/common")
target_compile_definitions(rn_tizen_icuuc
  PRIVATE U_COMMON_IMPLEMENTATION
  PUBLIC U_STATIC_IMPLEMENTATION U_CHARSET_IS_UTF8=1 U_HIDE_OBSOLETE_UTF_OLD_H=1)

add_library(rn_tizen_icui18n STATIC ${_icu_i18n_sources})
target_include_directories(rn_tizen_icui18n PRIVATE "${_icu_source}/common" "${_icu_source}/i18n")
target_compile_definitions(rn_tizen_icui18n PRIVATE U_I18N_IMPLEMENTATION)
target_link_libraries(rn_tizen_icui18n PUBLIC rn_tizen_icuuc)

foreach(_target rn_tizen_icuuc rn_tizen_icui18n)
  target_compile_options(${_target} PRIVATE -ffunction-sections -fdata-sections -w)
  set_target_properties(${_target} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
endforeach()
