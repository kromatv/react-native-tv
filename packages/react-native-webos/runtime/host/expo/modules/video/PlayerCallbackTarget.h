#pragma once

#include "PlayerLog.h"
#include "TizenPlayer.h"

#include "threading/MainThread.h"

#include <format>
#include <mutex>
#include <set>
#include <string>

namespace rntizen::expo {

struct TizenPlayer::CallbackTarget {
  CallbackTarget(std::weak_ptr<TizenPlayer> player, int number) : player(std::move(player)), number(number) {}

  template <typename Effect>
  void post(const char *callback, Effect effect) {
    {
      std::lock_guard lock(mutex);
      if (seen.emplace(callback).second) playerLog(number, std::format("{} arrives {}", callback, threadDescription()));
    }
    runOnMainThread([player = player, effect = std::move(effect)]() mutable {
      if (auto locked = player.lock()) locked->perform(std::move(effect));
    });
  }

  const std::weak_ptr<TizenPlayer> player;
  const int number;
  std::mutex mutex;
  std::set<std::string, std::less<>> seen;
};

} // namespace rntizen::expo
