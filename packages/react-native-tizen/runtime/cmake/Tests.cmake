# The host's tests: doctest cases in host/src/**/*.test.cpp, beside the code
# they test, in one executable that ctest runs. They run where they are built,
# so only a simulator build has them.
#
#   rn_tizen_add_host_tests(<target> SOURCES <test sources> HOST <object library>
#     ROOTS <React Native libraries> TIZEN_LIBRARIES <Tizen native API libraries>
#     INSTRUMENT <other targets compiling sources of runtime/>)

if(NOT RN_TIZEN_SIMULATOR)
  message(FATAL_ERROR "RN_TIZEN_TESTS needs RN_TIZEN_SIMULATOR: the tests run on the machine that builds them.")
endif()

FetchContent_Declare(doctest
  URL "https://github.com/doctest/doctest/releases/download/v2.5.3/doctest-v2.5.3.tar.gz"
  URL_HASH SHA256=eb4a39729d091e43d4d593888bf4d145ee9a0f0e13f7404484b6f2fb60d5d50f
  DOWNLOAD_DIR "${RN_TIZEN_DOWNLOAD_DIR}/doctest"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(doctest)

enable_testing()

function(rn_tizen_add_host_tests target)
  cmake_parse_arguments(PARSE_ARGV 1 _tests "" "HOST" "SOURCES;ROOTS;TIZEN_LIBRARIES;INSTRUMENT")

  set(_host_archive ${_tests_HOST}_archive)
  add_library(${_host_archive} STATIC EXCLUDE_FROM_ALL $<TARGET_OBJECTS:${_tests_HOST}>)
  target_link_libraries(${_host_archive} INTERFACE ${_tests_HOST})

  set(_react_native_archive ${_tests_HOST}_react_native)
  add_library(${_react_native_archive} STATIC EXCLUDE_FROM_ALL)
  rn_tizen_link_object_closure(${_react_native_archive} ${_tests_ROOTS})

  add_executable(${target} EXCLUDE_FROM_ALL ${_tests_SOURCES})
  target_link_libraries(${target} PRIVATE ${_host_archive} ${_react_native_archive} doctest::doctest_with_main)
  target_link_tizen_libraries(${target} ${_tests_TIZEN_LIBRARIES})
  target_compile_reactnative_options(${target} PRIVATE)
  target_compile_options(${target} PRIVATE -Wno-error)
  target_link_options(${target} PRIVATE "-Wl,--gc-sections")
  add_test(NAME ${target} COMMAND ${target})

  if(RN_TIZEN_COVERAGE)
    set(_instrument -fprofile-instr-generate -fcoverage-mapping)
    foreach(_instrumented IN LISTS _tests_HOST _tests_INSTRUMENT)
      target_compile_options(${_instrumented} PRIVATE ${_instrument})
    endforeach()
    target_link_options(${target} PRIVATE -fprofile-instr-generate)
    target_link_options(react-native-tizen PRIVATE -fprofile-instr-generate)
  endif()
endfunction()
