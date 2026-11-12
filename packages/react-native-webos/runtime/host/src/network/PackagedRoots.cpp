#include "PackagedRoots.h"

#include "../platform/AppPaths.h"

#include <unistd.h>

namespace rnwebos {

void trustPackagedRoots(CURL *easy) {
  static const std::string bundle = appPaths().resource + "cacert.pem";
#if RN_WEBOS_SIMULATOR
  // The simulator's libcurl is built with a default bundle, for an app staged without one.
  if (static const bool staged = access(bundle.c_str(), R_OK) == 0; !staged) return;
#endif
  curl_easy_setopt(easy, CURLOPT_CAINFO, bundle.c_str());
}

} // namespace rnwebos
