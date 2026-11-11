#pragma once

namespace rntizen {

/// The developer settings an app in development keeps across reloads, and
/// which of their changes the host must act on.
class DevSettingsState final {
 public:
  bool hotLoadingEnabled() const;
  /// True when Fast Refresh changed, which reloads the app, as Android does.
  bool setHotLoadingEnabled(bool enabled);
  /// True when LogBox's surface must start or stop.
  bool setLogBoxShown(bool shown);
  /// A new instance starts with no LogBox surface.
  void instanceStarted();

 private:
  bool hotLoadingEnabled_{true};
  bool logBoxShown_{false};
};

} // namespace rntizen
