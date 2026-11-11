#pragma once

#include "PlayerBackend.h"

#include <memory>

namespace rnwebos::expo {

/// The backend that plays on this device: the TV's media pipeline where the
/// Luna bus answers, the simulated one everywhere else. Main thread only.
std::unique_ptr<PlayerBackend> makePlayerBackend(PlayerBackendListener &listener, int number);

} // namespace rnwebos::expo
