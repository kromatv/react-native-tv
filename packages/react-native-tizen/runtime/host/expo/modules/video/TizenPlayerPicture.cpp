#include "TizenPlayer.h"

#include "NextFrame.h"
#include "PlayerLog.h"

#include "HostWindow.h"
#include "threading/MainThread.h"

#include <chrono>
#include <format>
#include <future>
#include <memory>
#include <optional>

namespace rntizen::expo {

namespace {

constexpr double kProbeSeconds = 1.0 / 30;
// Ecore's main loop may have quit before it drew the frame: the player thread must not wait for it forever.
constexpr auto kHoleCloseWait = std::chrono::milliseconds(250);

} // namespace

bool TizenPlayer::hasPicture() const {
  return display_.bound && display_.visible && picture_.watch.shown();
}

void TizenPlayer::followPicture() {
  auto &watch = picture_.watch;
  const bool running =
      handle_ != nullptr && phase_ == Phase::Ready && playing_ && !seek_.inFlight && !seek_.pending;
  if (!running) {
    watch.hold();
  } else if (!watch.shown() && !watch.due()) {
    int position = 0;
    player_get_play_position(handle_, &position);
    watch.expect(position, PictureWatch::Clock::now());
    playerLog(number_, std::format("picture due once playback moves on from {} ms", position));
  }
  if (watch.due() == (picture_.probe != nullptr)) return;
  if (watch.due()) {
    picture_.probe = ecore_timer_add(kProbeSeconds, pictureTick, this);
  } else {
    ecore_timer_del(picture_.probe);
    picture_.probe = nullptr;
  }
}

Eina_Bool TizenPlayer::pictureTick(void *data) {
  using enum PictureWatch::Arrival;
  auto &self = *static_cast<TizenPlayer *>(data);
  int position = 0;
  const bool told = player_get_play_position(self.handle_, &position) == PLAYER_ERROR_NONE;
  const auto arrival =
      self.picture_.watch.sample(told ? std::optional(position) : std::nullopt, PictureWatch::Clock::now());
  if (arrival == NotYet) return ECORE_CALLBACK_RENEW;
  self.picture_.probe = nullptr;
  if (arrival == Moved) {
    playerLog(self.number_, std::format("picture: playback reached {} ms", position));
  } else {
    playerLog(self.number_, "picture assumed: the position did not move in two seconds of playing", LogLevel::Warning);
  }
  if (self.hasPicture()) self.notifyDisplayChange();
  return ECORE_CALLBACK_CANCEL;
}

void TizenPlayer::losePicture() {
  const bool had = hasPicture();
  picture_.watch.lose();
  if (picture_.probe != nullptr) ecore_timer_del(picture_.probe);
  picture_.probe = nullptr;
  if (!had) return;
  playerLog(number_, "picture gone");
  if (!display_.presentation) return;
  notifyDisplayChange();
  noteHoleClosing();
}

void TizenPlayer::noteHoleClosing() {
  auto drawn = std::make_shared<std::promise<void>>();
  picture_.holeClosed = drawn->get_future().share();
  afterNextFrame(hostWindow(), [drawn, weak = weak_from_this()] {
    drawn->set_value();
    runOnMainThread([weak] {
      if (auto self = weak.lock()) self->perform([](TizenPlayer &player) { player.applyDisplay(); });
    });
  });
}

void TizenPlayer::awaitHoleClosed(const std::shared_future<void> &holeClosed) {
  if (holeClosed.valid()) holeClosed.wait_for(kHoleCloseWait);
}

} // namespace rntizen::expo
