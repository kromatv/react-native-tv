#pragma once

#include "LunaBus.h"

#include <functional>

namespace rnwebos {

/// What Back does on the app's first screen, when nothing in the app took it
/// (docs/reference/webos-window-input.md, sections 8 and 15).
enum class BackAtRoot {
  /// Shows the TV's Home over the app, which keeps running behind it: LG's
  /// checklist asks that Back on the entry page "must display the Home screen".
  ShowHome,
  /// Ends the app, and the TV shows what it shows when an app closes.
  Exit,
};

inline constexpr BackAtRoot kBackAtRoot = BackAtRoot::ShowHome;

/// Does what `behaviour` says. `exitApp` ends the app; ShowHome calls it too
/// when the app manager cannot be asked or refuses to open Home.
void leaveOnBackAtRoot(LunaBus &bus, BackAtRoot behaviour, std::function<void()> exitApp);

} // namespace rnwebos
