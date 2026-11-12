#include "VoiceController.h"

#include "../logging/Log.h"
#include "ContentContext.h"
#include "MediaSessions.h"
#include "SearchUrl.h"

#include <folly/json.h>

#include <functional>
#include <map>

namespace rntizen {

namespace {

const std::map<std::string, std::string, std::less<>> kAppStates{
    {"home", "Home"},
    {"list", "List"},
    {"details", "Details"},
    {"player", "Player"},
    {"search", "Search"},
    {"settings", "Setting"},
};

bool runNatively(MediaSession &session, const VoiceCommand &command) {
  if (command.type == "play") session.play();
  else if (command.type == "pause" || command.type == "stop") session.pause();
  else if (command.type == "restart") session.restart();
  else if (command.type == "skip") session.seekBy(command.seconds);
  else if (command.type == "seek") session.seekTo(command.position);
  else if (command.type == "subtitles") return session.setSubtitlesEnabled(command.enabled);
  else return false;
  return true;
}

} // namespace

VoiceController::VoiceController(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}

std::string VoiceController::appState() const {
  if (auto screen = dependencies_.state->screen()) {
    const auto state = kAppStates.find(*screen);
    if (state != kAppStates.end()) return state->second;
  }
  const auto session = currentMediaSession();
  return session && session->isPlaying() ? "Player" : "List";
}

std::string VoiceController::contentContext() const {
  auto *root = dependencies_.rootView();
  if (root == nullptr) return "[]";
  return folly::toJson(rntizen::contentContext(labelledViews(*root)));
}

bool VoiceController::handle(const std::string &json) {
  const auto command = parseVoiceCommand(json);
  if (!command) return false;
  if (command->type == "search") return search(*command);
  if (command->type == "selectTitle" || command->type == "selectIndex") return select(*command);
  return transport(*command);
}

bool VoiceController::transport(const VoiceCommand &command) {
  if (dependencies_.state->listening("command")) {
    dependencies_.emitDeviceEvent(kCommandEvent, command.payload());
    return true;
  }
  const auto session = currentMediaSession();
  return session != nullptr && runNatively(*session, command);
}

bool VoiceController::select(const VoiceCommand &command) {
  auto *root = dependencies_.rootView();
  if (root == nullptr) return false;
  const auto views = labelledViews(*root);
  const LabelledView *target = nullptr;
  if (command.type == "selectTitle") {
    target = findByTitle(views, command.title);
  } else if (!views.empty() && command.index != 0) {
    const auto position = command.index < 0 ? static_cast<int>(views.size()) + command.index : command.index - 1;
    if (position >= 0 && position < static_cast<int>(views.size())) target = &views[position];
  }
  if (target == nullptr) {
    log(LogLevel::Debug, "Voice: nothing on screen is labelled " + (command.title.empty() ? std::to_string(command.index) : command.title));
    return false;
  }
  log(LogLevel::Info, "Voice: pressing \"" + target->label + "\"");
  return tapView(*target);
}

bool VoiceController::search(const VoiceCommand &command) {
  if (dependencies_.state->listening("search")) {
    dependencies_.emitDeviceEvent(kSearchEvent, command.payload());
    return true;
  }
  const auto &searchUrl = dependencies_.state->searchUrl();
  if (!searchUrl) return false;
  const auto url = fillSearchUrl(*searchUrl, command.search);
  log(LogLevel::Debug, "Voice: opening " + url);
  dependencies_.openUrl(url);
  return true;
}

} // namespace rntizen
