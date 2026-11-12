#include "TizenPlayer.h"

#include "PlayerLog.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

namespace rntizen::expo {

void TizenPlayer::applySettings() {
  player_set_mute(handle_, settings_.muted);
  player_set_volume(handle_, static_cast<float>(settings_.volume), static_cast<float>(settings_.volume));
  player_set_looping(handle_, settings_.looping);
}

void TizenPlayer::applyHeaders(const SourceHeaders &headers) {
  for (const auto &[name, value] : headers) {
    std::string lower = name;
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
    const auto size = static_cast<int>(value.size());
    if (lower == "user-agent") {
      playerLog(number_, "User-Agent " + playerErrorName(player_set_streaming_user_agent(handle_, value.c_str(), size)));
    } else if (lower == "cookie") {
      playerLog(number_, "Cookie " + playerErrorName(player_set_streaming_cookie(handle_, value.c_str(), size)));
    } else {
      playerLog(
          number_,
          std::format("header {} dropped: the player takes only User-Agent and Cookie", name),
          LogLevel::Warning);
    }
  }
}

void TizenPlayer::setMuted(bool muted) {
  settings_.muted = muted;
  if (handle_ != nullptr) player_set_mute(handle_, muted);
}

void TizenPlayer::setVolume(double volume) {
  settings_.volume = std::clamp(volume, 0.0, 1.0);
  if (handle_ == nullptr) return;
  const auto level = static_cast<float>(settings_.volume);
  player_set_volume(handle_, level, level);
}

void TizenPlayer::setLooping(bool looping) {
  settings_.looping = looping;
  if (handle_ != nullptr) player_set_looping(handle_, looping);
}

void TizenPlayer::setPlaybackRate(double rate) {
  settings_.rate = rate;
  if (handle_ == nullptr || phase_ != Phase::Ready) return;
  playerLog(
      number_,
      std::format(
          "playback rate {} {}", decimal(rate, 2), playerErrorName(player_set_playback_rate(handle_, float(rate)))));
}

void TizenPlayer::setPrebuffer(double seconds) {
  settings_.prebufferMs = seconds > 0 ? std::max(1000, static_cast<int>(std::lround(seconds * 1000))) : 0;
}

void TizenPlayer::setMaxResolution(int width, int height) {
  if (handle_ == nullptr) return;
  playerLog(
      number_,
      std::format(
          "max variant {}x{} {}",
          width,
          height,
          playerErrorName(player_set_max_adaptive_variant_limit(handle_, -1, width, height))));
}

} // namespace rntizen::expo
