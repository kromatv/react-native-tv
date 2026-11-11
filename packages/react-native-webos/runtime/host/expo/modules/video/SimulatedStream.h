#pragma once

#include "PlayerModel.h"

#include <string>

namespace rnwebos::expo {

/// The stream SimulatedPlayer pretends a URI holds. Every URI plays as the
/// default; a fragment `#sim:` followed by `name=value` pairs joined with `&`
/// changes it, as in `https://example.com/a.mp4#sim:duration=12&error-at=5`:
/// `duration` (seconds), `size` (`1280x720`), `audio` and `text` (languages
/// joined with `+`, empty for none), `load` (seconds until it is ready),
/// `error` (an error code of the TV's player the load fails with) and
/// `error-at` (the position at which playback fails).
struct SimulatedStream {
  LoadedMedia media;
  double loadDelay{0.3};
  int loadError{0};
  double errorAt{-1};
};

SimulatedStream simulatedStreamFor(const std::string &uri);

} // namespace rnwebos::expo
