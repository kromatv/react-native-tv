#pragma once

#include <cxxreact/JSBigString.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace rntizen {

/// A JavaScript bundle, plain source or Hermes bytecode, and the URL it is reported under.
struct Bundle {
  std::unique_ptr<const facebook::react::JSBigString> script;
  std::string sourceURL;
};

/// The packaged bundle is not where the config says.
class BundleError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Reads a packaged bundle from disk; throws BundleError if it is missing.
Bundle loadBundleFile(const std::string &path);

/// Downloads a bundle from a Metro or Expo dev server; throws std::runtime_error
/// with the server's reply when it is not a 200.
Bundle downloadBundle(const std::string &url);

} // namespace rntizen
