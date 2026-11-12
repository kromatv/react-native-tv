# Hermes, built from source against React Native's own JSI. Its bytecode
# compiler for the build machine comes from a separate host build, whose
# ImportHostCompilers.cmake `rn-tizen` passes in.

set(RN_TIZEN_HERMES_HOST_COMPILERS "" CACHE FILEPATH
  "ImportHostCompilers.cmake from a Hermes build for this machine")
if(NOT EXISTS "${RN_TIZEN_HERMES_HOST_COMPILERS}")
  message(FATAL_ERROR
    "RN_TIZEN_HERMES_HOST_COMPILERS must point at a host Hermes build's ImportHostCompilers.cmake. "
    "`rn-tizen build-runtime` builds one and passes it.")
endif()

set(IMPORT_HOST_COMPILERS "${RN_TIZEN_HERMES_HOST_COMPILERS}" CACHE FILEPATH "" FORCE)
set(JSI_DIR "${REACT_NATIVE_DIR}/ReactCommon/jsi" CACHE PATH "" FORCE)
# The ICU Icu.cmake builds, not the TV's: a Tizen 9 TV ships ICU 74 and a
# Tizen 10 one ICU 76, so linking either would tie the package to one year.
set(HERMES_UNICODE_LITE OFF CACHE BOOL "" FORCE)
set(ICU_FOUND 1 CACHE BOOL "" FORCE)
set(ICU_LIBRARIES rn_tizen_icui18n rn_tizen_icuuc)
set(HERMES_IS_MOBILE_BUILD ON CACHE BOOL "" FORCE)
set(HERMES_ALLOW_BOOST_CONTEXT 0 CACHE STRING "" FORCE)
set(HERMES_ENABLE_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(HERMES_ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
# React Native DevTools debugs through Hermes' debugger, which costs every
# call a check: the development runtime alone has it (docs/decisions.md).
set(HERMES_ENABLE_DEBUGGER ${RN_TIZEN_DEVELOPMENT} CACHE BOOL "" FORCE)
set(HERMES_ENABLE_INTL ON CACHE BOOL "" FORCE)

add_subdirectory("${hermes_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/hermes" EXCLUDE_FROM_ALL)
# Hermes links ICU to the archive and not to the objects it compiles from.
target_link_libraries(hermesPlatformIntl_obj ${ICU_LIBRARIES})
# Its ICU toLocaleUpperCase and toLocaleLowerCase return the words "uppered"
# and "lowered": compiled under other names, they make way for ICU's own.
set_source_files_properties("${hermes_SOURCE_DIR}/lib/Platform/Intl/PlatformIntlICU.cpp"
  TARGET_DIRECTORY hermesPlatformIntl_obj
  PROPERTIES COMPILE_DEFINITIONS
    "toLocaleLowerCase=hermesStubToLocaleLowerCase;toLocaleUpperCase=hermesStubToLocaleUpperCase")
target_sources(hermesPlatformIntl_obj PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../intl/CaseMapping.cpp")

# Hermes defines the `jsi` target ReactCommon also defines; keep Hermes' and add
# the sources React Native's version compiles on top of it.
target_sources(jsi PRIVATE
  "${REACT_NATIVE_DIR}/ReactCommon/jsi/jsi/JSIDynamic.cpp"
  "${REACT_NATIVE_DIR}/ReactCommon/jsi/jsi/jsilib-posix.cpp")
target_link_libraries(jsi PUBLIC folly_runtime glog)

add_library(hermes-engine::hermesvm ALIAS hermesvm_a)
