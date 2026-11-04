#pragma once

#include "../views/ComponentView.h"
#include "TvState.h"
#include "VoiceController.h"
#include "VoiceScreen.test.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rntizen::testing {

/// A device event the controller emitted.
struct Event {
  std::string name;
  folly::dynamic payload;
};

/// A VoiceController on its own TvState, reading `screen`'s views, keeping
/// the device events it emits and the URLs it opens.
struct Voice {
  explicit Voice(std::optional<std::string> searchUrl = std::nullopt, VoiceScreen *screen = nullptr)
      : state(std::make_shared<TvState>(true, std::move(searchUrl))),
        controller({
            .state = state,
            .rootView = [screen]() -> ComponentView * { return screen != nullptr ? &screen->root() : nullptr; },
            .emitDeviceEvent =
                [this](const std::string &name, folly::dynamic payload) {
                  events.push_back({.name = name, .payload = std::move(payload)});
                },
            .openUrl = [this](const std::string &url) { openedUrls.push_back(url); },
        }) {}

  Voice(const Voice &) = delete;
  Voice &operator=(const Voice &) = delete;

  bool handle(const folly::dynamic &command) {
    return controller.handle(folly::toJson(command));
  }

  std::shared_ptr<TvState> state;
  std::vector<Event> events;
  std::vector<std::string> openedUrls;
  VoiceController controller;
};

} // namespace rntizen::testing
