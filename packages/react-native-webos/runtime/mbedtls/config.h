// What the runtime's TLS needs of Mbed TLS, applied on top of its default
// configuration: a client over TCP, called from several threads, that keeps
// nothing on disk.

// "mbedTLS can be used safely in a multi-threaded environment provided that
// mbedTLS is compiled with MBEDTLS_THREADING_C enabled" (curl,
// docs/libcurl/libcurl-thread.md); the host runs transfers on several threads.
#define MBEDTLS_THREADING_C
#define MBEDTLS_THREADING_PTHREAD

// Mbed TLS reads /dev/urandom unless the kernel headers number getrandom, and
// the webOS SDK's are Linux 3.10's. entropy.c asks the C library instead.
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_PROTO_DTLS
#undef MBEDTLS_SSL_DTLS_ANTI_REPLAY
#undef MBEDTLS_SSL_DTLS_HELLO_VERIFY
#undef MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#undef MBEDTLS_SSL_DTLS_CONNECTION_ID
#undef MBEDTLS_SSL_COOKIE_C
#undef MBEDTLS_SSL_CACHE_C
#undef MBEDTLS_SSL_TICKET_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_DEBUG_C
