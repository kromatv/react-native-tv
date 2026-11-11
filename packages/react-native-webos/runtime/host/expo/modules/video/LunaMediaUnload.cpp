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
  int unloading{0};
  std::deque<std::function<void()>> waiting;
};

Gate &gate() {
  static Gate instance;
  return instance;
}

struct Unload {
  std::unique_ptr<LunaConnection::Subscription> events;
  LunaEventSink sink;
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
    auto &state = gate();
    if (--state.unloading > 0) return;
    auto waiting = std::exchange(state.waiting, {});
    for (auto &task : waiting) task();
  });
}

} // namespace

void unloadMedia(
    LunaConnection &bus,
    const std::string &mediaId,
    std::unique_ptr<LunaConnection::Subscription> events,
    LunaEventSink sink,
    int player) {
  ++gate().unloading;
  auto unload = std::make_shared<Unload>();
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

void afterUnloads(std::function<void()> task) {
  if (gate().unloading == 0) {
    task();
    return;
  }
  gate().waiting.push_back(std::move(task));
}

} // namespace rnwebos::expo
