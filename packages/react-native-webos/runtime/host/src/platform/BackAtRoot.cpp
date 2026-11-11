#include "BackAtRoot.h"

#include "../logging/Log.h"

#include <utility>

namespace rnwebos {

namespace {

constexpr const char *kLaunch = "luna://com.webos.applicationManager/launch";

bool succeeded(const folly::dynamic &reply) {
  if (!reply.isObject()) return false;
  const auto *returnValue = reply.get_ptr("returnValue");
  return returnValue != nullptr && returnValue->isBool() && returnValue->getBool();
}

} // namespace

void leaveOnBackAtRoot(LunaBus &bus, BackAtRoot behaviour, const std::function<void()> &exitApp) {
  if (behaviour == BackAtRoot::Exit) {
    exitApp();
    return;
  }
  const folly::dynamic home = folly::dynamic::object("id", "com.webos.app.home")(
      "params", folly::dynamic::object("launchType", "homeKey"));
  const bool asked = bus.call(kLaunch, home, [exitApp](const folly::dynamic &reply) {
    if (succeeded(reply)) return;
    log(LogLevel::Info, "The app manager did not open Home on Back: leaving the app instead");
    exitApp();
  });
  if (!asked) exitApp();
}

} // namespace rnwebos
