#pragma once

#include "../views/ComponentView.h"
#include "TvState.h"
#include "VoiceCommand.h"

#include <folly/dynamic.h>

#include <functional>
#include <memory>
#include <string>

namespace rntizen {

/// Answers Samsung's voice service for the running app: the app state and the
/// titles on screen before each utterance, then the command itself. Transport
/// commands go to JavaScript's `command` listeners, else to the playing
/// player; a search to the `search` listeners, else to the search URL; a
/// selection presses the view with that label. Main thread only.
class VoiceController final {
 public:
  struct Dependencies {
    std::shared_ptr<TvState> state;
    std::function<ComponentView *()> rootView;
    std::function<void(const std::string &event, folly::dynamic payload)> emitDeviceEvent;
    std::function<void(const std::string &url)> openUrl;
  };

  /// The device events JavaScript's `Voice` listens to.
  static constexpr const char *kSearchEvent = "rnTizenVoiceSearch";
  static constexpr const char *kCommandEvent = "rnTizenVoiceCommand";

  explicit VoiceController(Dependencies dependencies);

  /// Samsung's app state name: `Home`, `List`, `Details`, `Player`, `Search` or `Setting`.
  std::string appState() const;

  /// The labelled views on screen, as Samsung's content context JSON.
  std::string contentContext() const;

  /// Runs the command the shell sent; false leaves it to the TV's default.
  bool handle(const std::string &json);

 private:
  bool transport(const VoiceCommand &command);
  bool select(const VoiceCommand &command);
  bool search(const VoiceCommand &command);

  Dependencies dependencies_;
};

} // namespace rntizen
