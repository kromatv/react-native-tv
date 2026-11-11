#pragma once

#include "../HostConfig.h"
#include "ControlLink.h"

#include <memory>

namespace rntizen {

/// Streams the app's logs to the config's log endpoint and, in a development
/// build, opens its control link there. A release build has no endpoint, so
/// nothing opens and no screenshot can be taken; null is returned.
std::unique_ptr<ControlLink> openDevelopmentLinks(const HostConfig &config);

} // namespace rntizen
