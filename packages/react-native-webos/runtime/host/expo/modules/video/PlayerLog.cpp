#include "PlayerLog.h"

#include "PlayerModel.h"

#include <format>

namespace rnwebos::expo {

void playerLog(int number, const std::string &message, LogLevel level) {
  log(level, std::format("[video {}] {}", number, message));
}

std::string decimal(double value, int digits) {
  return std::format("{:.{}f}", value, digits);
}

const char *statusName(PlayerStatus status) {
  using enum PlayerStatus;
  switch (status) {
    case Idle:
      return "idle";
    case Loading:
      return "loading";
    case ReadyToPlay:
      return "readyToPlay";
    case Error:
      return "error";
  }
  return "idle";
}

ContentFit contentFitFrom(std::string_view name) {
  using enum ContentFit;
  if (name == "cover") return Cover;
  if (name == "fill") return Fill;
  return Contain;
}

} // namespace rnwebos::expo
