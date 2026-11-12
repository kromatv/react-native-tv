#include "LunaMediaUnload.h"

#include "LunaMediaEvents.h"
#include "PipelineQuestions.h"
#include "PlayerLog.h"

#include "platform/MainLoop.h"

#include <deque>
#include <utility>

namespace rnwebos::expo {

namespace {

struct Gate {
  int held{0};
  std::deque<std::function<void()>> waiting;
};

Gate &gate() noexcept {
  static Gate instance;
  return instance;
}

void runWaiting() {
  const auto waiting = std::exchange(gate().waiting, {});
  for (const auto &task : waiting) task();
}

void leave() {
  if (--gate().held == 0) runWaiting();
}

struct Unload {
  std::unique_ptr<LunaConnection::Subscription> events;
  LunaEventSink sink;
  std::shared_ptr<void> plane;
  MainLoop::Id timer{0};
  bool done{false};
};

void finish(const std::shared_ptr<Unload> &unload) {
  if (std::exchange(unload->done, true)) return;
  MainLoop::get().removeTimer(unload->timer);
  // Posted: this runs inside the sink it is about to clear.
  MainLoop::get().post([unload] {
    *unload->sink = nullptr;
    unload->events.reset();
    unload->plane.reset();
    leave();
  });
}

struct Hold {
  Hold() = default;
  Hold(const Hold &) = delete;
  Hold &operator=(const Hold &) = delete;
  Hold(Hold &&) = delete;
  Hold &operator=(Hold &&) = delete;
  // A reply dropped unanswered, as when the bus closes, must not hold the loads that come after.
  ~Hold() {
    if (!released) --gate().held;
  }

  void release() {
    if (!std::exchange(released, true)) leave();
  }

  MainLoop::Id timer{0};
  bool released{false};
};

} // namespace

void unloadMedia(
    LunaConnection &bus,
    const std::string &mediaId,
    std::unique_ptr<LunaConnection::Subscription> events,
    LunaEventSink sink,
    std::shared_ptr<void> plane,
    int player) {
  ++gate().held;
  auto unload = std::make_shared<Unload>();
  unload->plane = std::move(plane);
  unload->events = std::move(events);
  unload->sink = sink ? std::move(sink) : std::make_shared<std::function<void(const folly::dynamic &)>>();
  *unload->sink = [unload, player](const folly::dynamic &message) {
    if (parseLunaMediaEvent(message).kind != LunaMediaEvent::Kind::UnloadCompleted) return;
    playerLog(player, "unloadCompleted");
    finish(unload);
  };
  unload->timer = MainLoop::get().addTimer(pipeline::kUnloadWait, [unload, player] {
    pipeline::openQuestion(player, "no unloadCompleted after unload: the next load goes ahead");
    finish(unload);
  });
  bus.call("luna://com.webos.media/unload", folly::dynamic::object("mediaId", mediaId));
}

std::function<void()> holdLoads(int player) {
  ++gate().held;
  auto hold = std::make_shared<Hold>();
  hold->timer = MainLoop::get().addTimer(pipeline::kLoadReplyWait, [waiting = std::weak_ptr(hold), player] {
    const auto unanswered = waiting.lock();
    if (!unanswered || unanswered->released) return;
    pipeline::openQuestion(player, "no reply to load: the next load goes ahead");
    unanswered->release();
  });
  return [hold] {
    if (hold->released) return;
    MainLoop::get().removeTimer(hold->timer);
    hold->release();
  };
}

void afterUnloads(std::function<void()> task) {
  gate().waiting.push_back(std::move(task));
  if (gate().held == 0) runWaiting();
}

} // namespace rnwebos::expo
