# Targets for React Native's C++ dependencies, under the names ReactCommon's
# CMakeLists link against. The source lists follow ReactAndroid's
# src/main/jni/third-party, which is the subset React Native compiles.

add_library(boost INTERFACE)
target_include_directories(boost SYSTEM INTERFACE "${boost_SOURCE_DIR}")

add_library(fast_float INTERFACE)
target_include_directories(fast_float SYSTEM INTERFACE "${fast_float_SOURCE_DIR}/include")

add_library(fmt STATIC "${fmt_SOURCE_DIR}/src/format.cc")
target_include_directories(fmt SYSTEM PUBLIC "${fmt_SOURCE_DIR}/include")

set(_double_conversion_include "${CMAKE_CURRENT_BINARY_DIR}/double-conversion/include")
file(GLOB _double_conversion_headers "${double_conversion_SOURCE_DIR}/src/*.h")
file(COPY ${_double_conversion_headers} DESTINATION "${_double_conversion_include}/double-conversion")
add_library(double-conversion STATIC
  "${double_conversion_SOURCE_DIR}/src/bignum.cc"
  "${double_conversion_SOURCE_DIR}/src/bignum-dtoa.cc"
  "${double_conversion_SOURCE_DIR}/src/cached-powers.cc"
  "${double_conversion_SOURCE_DIR}/src/diy-fp.cc"
  "${double_conversion_SOURCE_DIR}/src/double-conversion.cc"
  "${double_conversion_SOURCE_DIR}/src/fast-dtoa.cc"
  "${double_conversion_SOURCE_DIR}/src/fixed-dtoa.cc"
  "${double_conversion_SOURCE_DIR}/src/strtod.cc")
target_include_directories(double-conversion SYSTEM PUBLIC "${_double_conversion_include}")

set(_glog_exported "${CMAKE_CURRENT_BINARY_DIR}/glog/exported")
set(ac_cv_have_unistd_h 1)
set(ac_cv_have_stdint_h 1)
set(ac_cv_have_systypes_h 1)
set(ac_cv_have_inttypes_h 1)
set(ac_cv_have_libgflags 0)
set(ac_google_start_namespace "namespace google {")
set(ac_cv_have_uint16_t 1)
set(ac_cv_have_u_int16_t 1)
set(ac_cv_have___uint16 0)
set(ac_google_end_namespace "}")
set(ac_cv_have___builtin_expect 1)
set(ac_google_namespace google)
set(ac_cv___attribute___noinline "__attribute__ ((noinline))")
set(ac_cv___attribute___noreturn "__attribute__ ((noreturn))")
set(ac_cv___attribute___printf_4_5 "__attribute__((__format__ (__printf__, 4, 5)))")
foreach(_header logging raw_logging stl_logging vlog_is_on)
  configure_file("${glog_SOURCE_DIR}/src/glog/${_header}.h.in" "${_glog_exported}/glog/${_header}.h" @ONLY)
endforeach()
configure_file("${glog_SOURCE_DIR}/src/glog/log_severity.h" "${_glog_exported}/glog/log_severity.h" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/third-party/glog-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/glog/config.h" @ONLY)

add_library(glog STATIC
  "${glog_SOURCE_DIR}/src/demangle.cc"
  "${glog_SOURCE_DIR}/src/logging.cc"
  "${glog_SOURCE_DIR}/src/raw_logging.cc"
  "${glog_SOURCE_DIR}/src/signalhandler.cc"
  "${glog_SOURCE_DIR}/src/symbolize.cc"
  "${glog_SOURCE_DIR}/src/utilities.cc"
  "${glog_SOURCE_DIR}/src/vlog_is_on.cc")
target_include_directories(glog PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/glog" "${glog_SOURCE_DIR}/src")
target_include_directories(glog SYSTEM PUBLIC "${_glog_exported}")
target_compile_options(glog PRIVATE -Wno-sign-compare -Wno-deprecated-declarations -DNDEBUG -DHAVE_PREAD=1)

# ReactCommon links Android's glog_init and liblog by name; neither exists here.
add_library(glog_init INTERFACE)
target_link_libraries(glog_init INTERFACE glog)
add_library(log INTERFACE)

set(RN_TIZEN_FOLLY_FLAGS
  -DFOLLY_NO_CONFIG=1
  -DFOLLY_HAVE_CLOCK_GETTIME=1
  -DFOLLY_CFG_NO_COROUTINES=1
  -DFOLLY_MOBILE=1
  -DFOLLY_HAVE_RECVMMSG=1
  -DFOLLY_HAVE_PTHREAD=1
  -DFOLLY_HAVE_XSI_STRERROR_R=0)

set(_folly_sources
  Conv.cpp Demangle.cpp FileUtil.cpp Format.cpp ScopeGuard.cpp SharedMutex.cpp String.cpp
  Unicode.cpp concurrency/CacheLocality.cpp container/detail/F14Table.cpp
  detail/FileUtilDetail.cpp detail/Futex.cpp detail/SplitStringSimd.cpp detail/UniqueInstance.cpp
  hash/SpookyHashV2.cpp json/dynamic.cpp json/json_pointer.cpp json/json.cpp lang/CString.cpp
  lang/Exception.cpp lang/SafeAssert.cpp lang/ToAscii.cpp memory/detail/MallocImpl.cpp
  net/NetOps.cpp portability/SysUio.cpp synchronization/SanitizeThread.cpp
  synchronization/ParkingLot.cpp system/AtFork.cpp system/ThreadId.cpp)
list(TRANSFORM _folly_sources PREPEND "${folly_SOURCE_DIR}/folly/")

add_library(folly_runtime STATIC ${_folly_sources})
target_include_directories(folly_runtime SYSTEM PUBLIC "${folly_SOURCE_DIR}")
target_compile_options(folly_runtime PUBLIC ${RN_TIZEN_FOLLY_FLAGS})
target_compile_options(folly_runtime PRIVATE -fexceptions -frtti -Wno-sign-compare)
target_link_libraries(folly_runtime PUBLIC glog double-conversion boost fmt fast_float)
