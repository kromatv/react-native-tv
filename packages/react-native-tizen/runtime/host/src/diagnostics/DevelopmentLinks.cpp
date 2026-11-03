#include "DevelopmentLinks.h"

#include "../logging/Log.h"
#include "../logging/NetworkLogSink.h"

namespace rntizen {

std::unique_ptr<ControlLink> openDevelopmentLinks(const HostConfig &config) {
  if (!config.logEndpoint) return nullptr;
  const auto &[host, port] = *config.logEndpoint;
  setLogSink(std::make_shared<NetworkLogSink>(host, port));
  if (!config.development) return nullptr;
  return std::make_unique<ControlLink>(host, port);
}

} // namespace rntizen
