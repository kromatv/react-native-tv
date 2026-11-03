cmake_minimum_required(VERSION 3.24)
project(react_native_tizen LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(RN_TIZEN_CCACHE ccache)
if(RN_TIZEN_CCACHE)
  set(CMAKE_C_COMPILER_LAUNCHER "${RN_TIZEN_CCACHE}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${RN_TIZEN_CCACHE}")
endif()

option(RN_TIZEN_SIMULATOR "Build for the Linux simulator in ../simulator instead of a TV" OFF)
option(RN_TIZEN_TESTS "Build the host's tests, rn-tizen-host-tests (simulator builds only)" OFF)
option(RN_TIZEN_COVERAGE "Instrument the host and its tests for llvm-cov" OFF)
# A development runtime talks to a dev server (host/devsupport); a release one
# carries none of it. The simulator only runs development builds.
option(RN_TIZEN_DEVELOPMENT "Build the development runtime, with the dev server tooling" ${RN_TIZEN_SIMULATOR})

set(REACT_NATIVE_DIR "" CACHE PATH "The react-native package the app depends on")
if(NOT EXISTS "${REACT_NATIVE_DIR}/ReactCommon")
  message(FATAL_ERROR "REACT_NATIVE_DIR must be a react-native package, got '${REACT_NATIVE_DIR}'")
endif()

find_package(Threads REQUIRED)

add_compile_definitions(RN_BUILDING)
include("${REACT_NATIVE_DIR}/ReactCommon/cmake-utils/react-native-flags.cmake")

if(RN_TIZEN_SIMULATOR)
  include(cmake/Simulator.cmake)
endif()
include(cmake/Dependencies.cmake)
include(cmake/ThirdParty.cmake)
include(cmake/Icu.cmake)
include(cmake/Hermes.cmake)
include(cmake/Codegen.cmake)
include(cmake/ReactCommon.cmake)
include(cmake/Expo.cmake)
include(cmake/ObjectClosure.cmake)
if(NOT RN_TIZEN_SIMULATOR)
  include(cmake/TizenNativeApi.cmake)
endif()
if(RN_TIZEN_TESTS)
  include(cmake/Tests.cmake)
endif()

add_subdirectory(platform)
add_subdirectory(host)
target_link_libraries(rn_tizen_host PUBLIC rn_tizen_icuuc)
target_link_libraries(react-native-tizen PRIVATE rn_tizen_icuuc)
include(cmake/Libraries.cmake)
