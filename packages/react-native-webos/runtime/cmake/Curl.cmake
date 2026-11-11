# libcurl and Mbed TLS, compiled in. A TV's own libcurl changes SONAME at webOS
# 5, lacks calls the host makes before webOS 7, and trusts a root list as old
# as its firmware; its OpenSSL has four SONAMEs
# (docs/reference/webos-native-toolchain.md, section 7). The simulator links
# the same two, so both run one HTTP and TLS stack. Only HTTP and HTTPS:
# WebSocket framing is the host's own, over a CONNECT_ONLY transfer.

set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE BOOL "" FORCE)
set(MBEDTLS_USER_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/../mbedtls/config.h" CACHE FILEPATH "" FORCE)
add_subdirectory("${mbedtls_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/mbedtls" EXCLUDE_FROM_ALL)
target_sources(mbedcrypto PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../mbedtls/entropy.c")

set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "" FORCE)
set(PICKY_COMPILER OFF CACHE BOOL "" FORCE)
# Nothing of the build machine or the sysroot is picked up unasked.
set(CURL_USE_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(CURL_USE_CMAKECONFIG OFF CACHE BOOL "" FORCE)
foreach(_library USE_NGHTTP2 USE_LIBIDN2 CURL_USE_LIBPSL CURL_USE_LIBSSH2 CURL_USE_OPENSSL ENABLE_UNIX_SOCKETS)
  set(${_library} OFF CACHE BOOL "" FORCE)
endforeach()
set(CURL_BROTLI OFF CACHE STRING "" FORCE)
set(CURL_ZSTD OFF CACHE STRING "" FORCE)
set(CURL_ZLIB ON CACHE STRING "" FORCE)
foreach(_feature
    DICT FILE FTP GOPHER IMAP LDAP LDAPS MQTT POP3 RTSP SMTP TELNET TFTP IPFS WEBSOCKETS
    ALTSVC HSTS DOH NETRC AWS KERBEROS_AUTH NEGOTIATE_AUTH MIME FORM_API HEADERS_API GETOPTIONS
    LIBCURL_OPTION PROGRESS_METER SHUFFLE_DNS BINDLOCAL)
  set(CURL_DISABLE_${_feature} ON CACHE BOOL "" FORCE)
endforeach()

set(CURL_ENABLE_SSL ON CACHE BOOL "" FORCE)
set(CURL_USE_MBEDTLS ON CACHE BOOL "" FORCE)
set(MBEDTLS_INCLUDE_DIR "${mbedtls_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)
# curl asks a linked Mbed TLS for DES, for NTLM alone; these are not built yet.
set(HAVE_MBEDTLS_DES_CRYPT_ECB 0 CACHE INTERNAL "")

# A TV app's roots are the cacert.pem its package ships, which the host names
# for every transfer. The simulator falls back to the same pinned file where
# the build downloaded it, for an app staged without one.
if(RN_WEBOS_SIMULATOR)
  set(CURL_CA_BUNDLE "${RN_WEBOS_cacert_FILE}" CACHE STRING "" FORCE)
else()
  set(CURL_CA_BUNDLE none CACHE STRING "" FORCE)
endif()
set(CURL_CA_PATH none CACHE STRING "" FORCE)

add_subdirectory("${curl_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/curl" EXCLUDE_FROM_ALL)

foreach(_target libcurl_static mbedtls mbedx509 mbedcrypto everest p256m)
  target_compile_options(${_target} PRIVATE -Os -w -ffunction-sections -fdata-sections)
endforeach()
