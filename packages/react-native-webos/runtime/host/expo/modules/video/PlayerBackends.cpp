#include "PlayerBackends.h"

#include "ExportedWindowPlane.h"
#include "LunaMediaPlayer.h"
#include "PipelineQuestions.h"
#include "PlayerLog.h"
#include "SimulatedPlayer.h"

#include "platform/LunaBus.h"
#include "platform/LunaClient.h"

#include <unistd.h>

#include <cstdlib>
#include <format>

namespace rnwebos::expo {

namespace {

std::string appId() {
  const char *id = std::getenv("APPID");
  return id != nullptr ? id : "";
}

// The connection is never closed: the pipeline unloads a media when the client
// that loaded it leaves the bus.
LunaConnection *connect() {
  for (const auto &name : pipeline::busNames(appId(), getpid())) {
    std::string refusal;
    if (auto client = LunaClient::registerAs(name, &refusal)) {
      log(LogLevel::Info, "[video] on the Luna bus as " + name);
      return client.release();
    }
    log(LogLevel::Debug, std::format("[video] open question: the Luna bus refused the name {}: {}", name, refusal));
  }
  auto &own = LunaBus::get();
  if (!own.isAvailable()) return nullptr;
  log(LogLevel::Debug, "[video] open question: calling com.webos.media as the app itself, which the TV may deny");
  return &own;
}

LunaConnection *mediaConnection() {
  static LunaConnection *const connection = connect();
  return connection;
}

} // namespace

std::unique_ptr<PlayerBackend> makePlayerBackend(PlayerBackendListener &listener, int number) {
  if (auto *bus = mediaConnection()) {
    return std::make_unique<LunaMediaPlayer>(*bus, std::make_unique<ExportedWindowPlane>(number), listener, appId(), number);
  }
  return std::make_unique<SimulatedPlayer>(listener, number);
}

} // namespace rnwebos::expo
