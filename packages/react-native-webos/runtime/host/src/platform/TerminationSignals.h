#pragma once

#include <functional>

namespace rnwebos {

/// Keeps SIGTERM and SIGINT from ending the process where they land. Call it
/// first in `main`: a thread started earlier does not inherit the block.
void blockTerminationSignals();

/// Calls `onTerminate` once, on a thread of its own, when SIGTERM or SIGINT
/// arrives: SIGTERM is how the TV's app manager closes an app that is not
/// registered or declares lifecycle interface 1, one second before SIGKILL
/// (docs/reference/webos-tv-platform.md, section 3.3). A second signal ends
/// the process at once. Needs `blockTerminationSignals` before it.
void watchTerminationSignals(std::function<void()> onTerminate);

} // namespace rnwebos
