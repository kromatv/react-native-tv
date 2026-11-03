#include "BundleLoader.h"

#include "../network/HttpGet.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace rntizen {

Bundle loadBundleFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) throw std::runtime_error("No bundle at " + path);
  std::ostringstream contents;
  contents << file.rdbuf();
  return Bundle{
      .script = std::make_unique<facebook::react::JSBigStdString>(contents.str()),
      .sourceURL = "file://" + path,
  };
}

Bundle downloadBundle(const std::string &url) {
  return Bundle{.script = std::make_unique<facebook::react::JSBigStdString>(httpGet(url)), .sourceURL = url};
}

} // namespace rntizen
