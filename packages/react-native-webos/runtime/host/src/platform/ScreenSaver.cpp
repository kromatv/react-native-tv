#include "ScreenSaver.h"

#include <SDL3/SDL_video.h>

#include <utility>

namespace rnwebos {

namespace {

constexpr const char *kRegister = "luna://com.webos.service.tvpower/power/registerScreenSaverRequest";
constexpr const char *kRespond = "luna://com.webos.service.tvpower/power/responseScreenSaverRequest";

} // namespace

std::optional<folly::dynamic> screenSaverAnswer(
    const folly::dynamic &request,
    const std::string &clientName,
    bool allowed) {
  if (!request.isObject()) return std::nullopt;
  const auto *timestamp = request.get_ptr("timestamp");
  if (timestamp == nullptr) return std::nullopt;
  return folly::dynamic::object("clientName", clientName)("ack", allowed)("timestamp", *timestamp);
}

std::unique_ptr<ScreenSaver> ScreenSaver::watch(LunaBus &bus, const std::string &appId) {
  auto saver = std::unique_ptr<ScreenSaver>(new ScreenSaver(bus, appId + ".wakelock"));
  saver->requests_ = bus.subscribe(
      kRegister,
      folly::dynamic::object("subscribe", true)("clientName", saver->clientName_),
      [self = saver.get()](const folly::dynamic &request) { self->answer(request); });
  return saver->requests_ != nullptr ? std::move(saver) : nullptr;
}

ScreenSaver::ScreenSaver(LunaBus &bus, std::string clientName) : bus_(bus), clientName_(std::move(clientName)) {}

void ScreenSaver::answer(const folly::dynamic &request) const {
  if (const auto reply = screenSaverAnswer(request, clientName_, SDL_ScreenSaverEnabled())) bus_.call(kRespond, *reply);
}

} // namespace rnwebos
