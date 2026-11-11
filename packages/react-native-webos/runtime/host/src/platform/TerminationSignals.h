#pragma once

#include <functional>

namespace rnwebos {

/// Keeps SIGTERM and SIGINT from ending the process where they land. Call it
/// first in `main`: a thread started earlier does not inherit the block.
void blockTerminationSignals();

/// Calls `onTerminate` once, on a thread of its own, at SIGTERM or SIGINT,
/// and ends the process at a second one. SIGTERM is how the TV's app manager
/// closes some apps, a second before SIGKILL (docs/reference/webos-tv-platform.md, section 3.3).
void watchTerminationSignals(std::function<void()> onTerminate);

} // namespace rnwebos
