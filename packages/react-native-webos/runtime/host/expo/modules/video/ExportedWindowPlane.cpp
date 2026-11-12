#include "ExportedWindowPlane.h"

#include "PipelineQuestions.h"
#include "PlayerLog.h"

#include <SDL3/SDL.h>

#include <format>
#include <utility>

namespace rnwebos::expo {

namespace {

#ifdef SDL_PLATFORM_WEBOS

std::string createWindow(int player) {
  const char *id = SDL_webOSCreateExportedWindow(SDL_WEBOS_EXPORTED_WINDOW_TYPE_VIDEO);
  if (id == nullptr) {
    playerLog(player, std::format("no exported window: {}", SDL_GetError()), LogLevel::Warning);
    return {};
  }
  // The apps that ship set a rectangle before they load: the whole window, until a view says where.
  SDL_webOSSetExportedWindow(id, nullptr, nullptr);
  playerLog(player, std::format("exported window {}", id));
  return id;
}

void placeWindow(const std::string &id, const PixelRect &source, const PixelRect &destination, int player) {
  SDL_Rect from{.x = source.x, .y = source.y, .w = source.width, .h = source.height};
  SDL_Rect to{.x = destination.x, .y = destination.y, .w = destination.width, .h = destination.height};
  const bool placed = SDL_webOSSetExportedWindow(id.c_str(), source.width > 0 ? &from : nullptr, &to);
  playerLog(
      player,
      std::format(
          "plane {},{} {}x{} from {},{} {}x{}: {}",
          to.x, to.y, to.w, to.h, from.x, from.y, from.w, from.h, placed ? "set" : SDL_GetError()),
      LogLevel::Debug);
}

void destroyWindow(const std::string &id) {
  SDL_webOSDestroyExportedWindow(id.c_str());
}

#else

std::string createWindow(int player) {
  playerLog(player, "no exported window: this build is not for a TV", LogLevel::Warning);
  return {};
}

void placeWindow(const std::string &, const PixelRect &, const PixelRect &, int) {
  // Only a TV's compositor has the plane.
}

void destroyWindow(const std::string &) {
  // Nothing was created.
}

#endif

} // namespace

ExportedWindowPlane::ExportedWindowPlane(int player) : player_(player) {}

ExportedWindowPlane::~ExportedWindowPlane() {
  if (!windowId_.empty()) destroyWindow(windowId_);
}

std::string ExportedWindowPlane::windowId() {
  if (!std::exchange(created_, true)) windowId_ = createWindow(player_);
  return windowId_;
}

void ExportedWindowPlane::show(Scene &, const PixelRect &source, const PixelRect &destination) {
  if (windowId().empty() || (source == source_ && destination == destination_)) return;
  if (destination_.width > 0 && !std::exchange(moved_, true)) {
    pipeline::openQuestion(player_, "the plane's rectangle changes while it shows: does the picture follow?");
  }
  source_ = source;
  destination_ = destination;
  placeWindow(windowId_, source, destination, player_);
}

// The view stops clearing its hole, so the app's own pixels cover the plane:
// no rectangle is known to hide it (an empty one, or one off the panel, may blank it).
void ExportedWindowPlane::hide() {
  destination_ = {};
}

} // namespace rnwebos::expo
