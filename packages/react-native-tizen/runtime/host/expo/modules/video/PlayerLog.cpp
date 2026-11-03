#include "PlayerLog.h"

#include "PlayerModel.h"

#include "threading/MainThread.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <thread>

namespace rntizen::expo {

void playerLog(int number, const std::string &message, LogLevel level) {
  log(level, "[video " + std::to_string(number) + "] " + message);
}

std::string playerErrorName(int code) {
  switch (code) {
    case PLAYER_ERROR_NONE:
      return "PLAYER_ERROR_NONE";
    case PLAYER_ERROR_OUT_OF_MEMORY:
      return "PLAYER_ERROR_OUT_OF_MEMORY";
    case PLAYER_ERROR_INVALID_PARAMETER:
      return "PLAYER_ERROR_INVALID_PARAMETER";
    case PLAYER_ERROR_NO_SUCH_FILE:
      return "PLAYER_ERROR_NO_SUCH_FILE";
    case PLAYER_ERROR_INVALID_OPERATION:
      return "PLAYER_ERROR_INVALID_OPERATION";
    case PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE:
      return "PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE";
    case PLAYER_ERROR_SEEK_FAILED:
      return "PLAYER_ERROR_SEEK_FAILED";
    case PLAYER_ERROR_INVALID_STATE:
      return "PLAYER_ERROR_INVALID_STATE";
    case PLAYER_ERROR_NOT_SUPPORTED_FILE:
      return "PLAYER_ERROR_NOT_SUPPORTED_FILE";
    case PLAYER_ERROR_INVALID_URI:
      return "PLAYER_ERROR_INVALID_URI";
    case PLAYER_ERROR_SOUND_POLICY:
      return "PLAYER_ERROR_SOUND_POLICY";
    case PLAYER_ERROR_CONNECTION_FAILED:
      return "PLAYER_ERROR_CONNECTION_FAILED";
    case PLAYER_ERROR_RESOURCE_LIMIT:
      return "PLAYER_ERROR_RESOURCE_LIMIT";
    case PLAYER_ERROR_PERMISSION_DENIED:
      return "PLAYER_ERROR_PERMISSION_DENIED";
    case PLAYER_ERROR_SERVICE_DISCONNECTED:
      return "PLAYER_ERROR_SERVICE_DISCONNECTED";
    case PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC:
      return "PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC";
    case PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC:
      return "PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC";
    case PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE:
      return "PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE";
    case PLAYER_ERROR_NOT_SUPPORTED_FORMAT:
      return "PLAYER_ERROR_NOT_SUPPORTED_FORMAT";
    case PLAYER_ERROR_NOT_AVAILABLE:
      return "PLAYER_ERROR_NOT_AVAILABLE";
    default: {
      std::ostringstream hex;
      hex << "player error 0x" << std::hex << static_cast<unsigned int>(code);
      return hex.str();
    }
  }
}

const char *playerStateName(player_state_e state) {
  switch (state) {
    case PLAYER_STATE_IDLE:
      return "IDLE";
    case PLAYER_STATE_READY:
      return "READY";
    case PLAYER_STATE_PLAYING:
      return "PLAYING";
    case PLAYER_STATE_PAUSED:
      return "PAUSED";
    default:
      return "NONE";
  }
}

std::string decimal(double value, int digits) {
  std::ostringstream text;
  text.imbue(std::locale::classic());
  text << std::fixed << std::setprecision(digits) << value;
  return text.str();
}

std::string millisSince(std::chrono::steady_clock::time_point start) {
  return decimal(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(), 1) + " ms";
}

std::string threadDescription() {
  std::ostringstream description;
  description << (isMainThread() ? "on the main loop" : "off the main loop") << " (thread "
              << std::this_thread::get_id() << ")";
  return description.str();
}

const char *statusName(PlayerStatus status) {
  switch (status) {
    case PlayerStatus::Idle:
      return "idle";
    case PlayerStatus::Loading:
      return "loading";
    case PlayerStatus::ReadyToPlay:
      return "readyToPlay";
    case PlayerStatus::Error:
      return "error";
  }
  return "idle";
}

ContentFit contentFitFrom(const std::string &name) {
  if (name == "cover") return ContentFit::Cover;
  if (name == "fill") return ContentFit::Fill;
  return ContentFit::Contain;
}

} // namespace rntizen::expo
