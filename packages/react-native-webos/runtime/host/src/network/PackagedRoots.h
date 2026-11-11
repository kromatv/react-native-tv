#pragma once

#include <curl/curl.h>

namespace rnwebos {

/// Makes a transfer verify servers against the root certificates the app's
/// package ships as `cacert.pem` in its resource directory. The runtime's TLS
/// is its own, not the TV's, so nothing else tells it which roots to trust.
/// Any thread.
void trustPackagedRoots(CURL *easy);

} // namespace rnwebos
