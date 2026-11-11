#include "PlayerCallbackTarget.h"
#include "PlayerLog.h"
#include "PlayerThread.h"
#include "TizenPlayer.h"

#include "threading/MainThread.h"

#include <chrono>
#include <utility>

namespace rntizen::expo {

namespace {

using Clock = std::chrono::steady_clock;

std::string between(Clock::time_point start, Clock::time_point end) {
  return decimal(std::chrono::duration<double, std::milli>(end - start).count(), 1) + " ms";
}

} // namespace

void TizenPlayer::perform(std::function<void(TizenPlayer &)> command) {
  if (lending_.lent) {
    lending_.deferred.push_back(std::move(command));
    return;
  }
  command(*this);
}

void TizenPlayer::create(std::function<void(TizenPlayer &)> then) {
  if (!target_) target_ = std::make_shared<CallbackTarget>(weak_from_this(), number_);
  auto result = std::make_shared<int>(PLAYER_ERROR_NONE);
  lend(
      [number = number_, result](player_h) {
        player_h handle = nullptr;
        const auto started = Clock::now();
        *result = player_create(&handle);
        playerLog(
            number,
            "player_create " + playerErrorName(*result) + " in " + millisSince(started) + ", " + threadDescription());
        return *result == PLAYER_ERROR_NONE ? handle : nullptr;
      },
      [result, then = std::move(then)](TizenPlayer &self) {
        if (self.handle_ == nullptr) {
          self.fail("This device has no media player: player_create returned " + playerErrorName(*result));
          return;
        }
        self.setCallbacks();
        self.bindDisplay();
        self.applySettings();
        playerLog(self.number_, "created");
        then(self);
      });
}

void TizenPlayer::lend(std::function<player_h(player_h)> work, std::function<void(TizenPlayer &)> resume) {
  lending_.lent = true;
  lending_.resume = std::move(resume);
  runOnPlayerThread([weak = weak_from_this(),
                     handle = std::exchange(handle_, nullptr),
                     work = std::move(work),
                     target = target_,
                     number = number_] {
    auto *back = work(handle);
    runOnMainThread([weak, back, target, number] {
      if (auto self = weak.lock()) {
        self->reclaim(back);
      } else {
        releaseOnPlayerThread(back, target, number, false);
      }
    });
  });
}

void TizenPlayer::reclaim(player_h handle) {
  handle_ = handle;
  lending_.lent = false;
  if (released_) {
    releaseHandle();
    return;
  }
  if (auto resume = std::exchange(lending_.resume, nullptr)) resume(*this);
  while (!lending_.lent && !lending_.deferred.empty()) {
    auto command = std::move(lending_.deferred.front());
    lending_.deferred.pop_front();
    command(*this);
  }
  if (!lending_.lent) applyDisplay();
}

void TizenPlayer::release() {
  if (released_) return;
  released_ = true;
  lending_.deferred.clear();
  if (!lending_.lent) releaseHandle();
}

void TizenPlayer::releaseHandle() {
  stopProgress();
  const bool preparing = phase_ == Phase::Preparing;
  phase_ = Phase::Closed;
  if (handle_ == nullptr) return;
  setVisible(false);
  releaseOnPlayerThread(std::exchange(handle_, nullptr), target_, number_, preparing);
  display_.bound = false;
  notifyDisplayChange();
}

void TizenPlayer::releaseOnPlayerThread(
    player_h handle, std::shared_ptr<CallbackTarget> target, int number, bool preparing) {
  if (handle == nullptr) return;
  runOnPlayerThread([handle, target = std::move(target), number, preparing] {
    const auto started = Clock::now();
    unsetCallbacks(handle);
    player_state_e current = PLAYER_STATE_NONE;
    player_get_state(handle, &current);
    if (current == PLAYER_STATE_PLAYING || current == PLAYER_STATE_PAUSED) player_stop(handle);
    const auto stopped = Clock::now();
    if (current != PLAYER_STATE_IDLE || preparing) player_unprepare(handle);
    const auto unprepared = Clock::now();
    const int destroyed = player_destroy(handle);
    playerLog(
        number,
        "released from " + std::string(playerStateName(current)) + ": " + playerErrorName(destroyed) + ", stop " +
            between(started, stopped) + ", unprepare " + between(stopped, unprepared) + ", destroy " +
            millisSince(unprepared) + ", " + threadDescription());
  });
}

} // namespace rntizen::expo
