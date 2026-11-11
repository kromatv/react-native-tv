#include "AppRegistration.h"

#include "../logging/Log.h"

#include <format>
#include <utility>

namespace rnwebos {

namespace {

constexpr const char *kRegisterNativeApp = "luna://com.webos.applicationManager/registerNativeApp";
constexpr const char *kRegisterApp = "luna://com.webos.applicationManager/registerApp";

std::string stringAt(const folly::dynamic &object, const char *key) {
  const auto *value = object.get_ptr(key);
  return value != nullptr && value->isString() ? value->getString() : "";
}

bool failed(const folly::dynamic &reply) {
  const auto *returnValue = reply.get_ptr("returnValue");
  return returnValue != nullptr && returnValue->isBool() && !returnValue->getBool();
}

} // namespace

std::unique_ptr<AppRegistration> AppRegistration::start(
    LunaBus &bus,
    std::string appId,
    int interfaceVersion,
    Events events) {
  auto registration = std::unique_ptr<AppRegistration>(new AppRegistration(bus, std::move(appId), std::move(events)));
  if (!registration->subscribe(interfaceVersion)) return nullptr;
  return registration;
}

AppRegistration::AppRegistration(LunaBus &bus, std::string appId, Events events)
    : bus_(bus), appId_(std::move(appId)), events_(std::move(events)) {}

bool AppRegistration::subscribe(int interfaceVersion) {
  interfaceVersion_ = interfaceVersion;
  subscription_ = bus_.subscribe(
      interfaceVersion == 1 ? kRegisterNativeApp : kRegisterApp,
      folly::dynamic::object("id", appId_),
      [this](const folly::dynamic &reply) { onReply(reply); });
  return subscription_ != nullptr;
}

void AppRegistration::onReply(const folly::dynamic &reply) {
  if (!reply.isObject()) return;
  if (failed(reply)) {
    log(LogLevel::Warning,
        std::format(
            "The app manager refused the registration under lifecycle interface {}: {}",
            interfaceVersion_,
            stringAt(reply, "errorText")));
    if (registered_ || std::exchange(triedOtherVersion_, true)) return;
    subscribe(interfaceVersion_ == 1 ? 2 : 1);
    return;
  }
  // Interface 1 names what happened in `message`, interface 2 in `event`.
  auto event = stringAt(reply, "event");
  if (event.empty()) event = stringAt(reply, "message");
  if (event == "registered") {
    registered_ = true;
    log(LogLevel::Info, std::format("Registered with the app manager, lifecycle interface {}", interfaceVersion_));
  } else if (event == "relaunch") {
    const auto *parameters = reply.get_ptr("parameters");
    const bool hasParameters = parameters != nullptr && parameters->isObject();
    events_.relaunch(hasParameters ? *parameters : folly::dynamic::object(), stringAt(reply, "reason"));
  } else if (event == "pause") {
    events_.pause();
  } else if (event == "close") {
    events_.close();
  }
}

} // namespace rnwebos
