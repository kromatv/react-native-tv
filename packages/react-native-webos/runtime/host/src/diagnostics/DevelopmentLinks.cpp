#include "DevelopmentLinks.h"

#include "../logging/Log.h"
#include "../logging/NetworkLogSink.h"

#include <utility>

namespace rnwebos {

std::unique_ptr<ControlLink> openDevelopmentLinks(const HostConfig &config, ControlLink::WindowCapture captureWindow) {
  if (!config.logEndpoint) return nullptr;
  const auto &[host, port] = *config.logEndpoint;
  setLogSink(std::make_shared<NetworkLogSink>(host, port, config.packageId));
  if (!config.development) return nullptr;
  return std::make_unique<ControlLink>(host, port, config.packageId, std::move(captureWindow));
}

} // namespace rnwebos
