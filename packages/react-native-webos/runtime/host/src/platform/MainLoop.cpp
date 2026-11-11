#include "MainLoop.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace rnwebos {

namespace {

using Clock = MainLoop::Clock;

struct Timer {
  Clock::time_point due;
  MainLoop::Task task;
};

int millisecondsUntil(Clock::time_point time, Clock::time_point now) {
  if (time <= now) return 0;
  const auto wait = std::chrono::ceil<std::chrono::milliseconds>(time - now).count();
  return static_cast<int>(std::min<std::int64_t>(wait, 60'000));
}

} // namespace

struct MainLoop::State {
  const std::thread::id mainThread{std::this_thread::get_id()};
  std::mutex mutex;
  std::deque<Task> posted;
  std::atomic<bool> frameRequested{false};
  std::atomic<bool> quitting{false};
  std::atomic<int> exitCode{0};
  std::atomic<bool> waking{false};

  Id nextId{1};
  std::map<Id, Timer> timers;
  std::map<Id, FrameCallback> frameCallbacks;
  Renderer renderer;
  EventHandler eventHandler;
  WaitObserver waitObserver;
  Clock::duration framePeriod{std::chrono::nanoseconds(16'666'667)};
  // The earliest the next frame may start.
  Clock::time_point nextFrame{};

  void wake() {
    if (waking.exchange(true)) return;
    SDL_Event event{};
    event.type = SDL_EVENT_USER;
    SDL_PushEvent(&event);
  }

  bool wantsFrame() const {
    return frameRequested.load() || !frameCallbacks.empty();
  }

  Clock::time_point nextTimer() const {
    auto next = Clock::time_point::max();
    for (const auto &[id, timer] : timers) next = std::min(next, timer.due);
    return next;
  }

  void dispatch(const SDL_Event &event) {
    if (event.type == SDL_EVENT_USER) return;
    if (eventHandler) eventHandler(event);
  }

  void runPosted() {
    std::deque<Task> tasks;
    {
      std::lock_guard lock(mutex);
      tasks.swap(posted);
    }
    for (auto &task : tasks) task();
  }

  void runTimers() {
    const auto now = Clock::now();
    std::vector<Id> due;
    for (const auto &[id, timer] : timers) {
      if (timer.due <= now) due.push_back(id);
    }
    for (const auto id : due) {
      auto found = timers.extract(id);
      if (!found.empty()) found.mapped().task();
    }
  }

  // Counted from a timer's late wake, every period stretched: 51 frames a second for 60.
  // A frame over a period late was held by the display's swap, so the next starts at once.
  void runPacedFrame() {
    const auto now = Clock::now();
    nextFrame = std::max(nextFrame + framePeriod, now);
    runFrame(now);
  }

  void runFrame(Clock::time_point now) {
    frameRequested.store(false);
    std::vector<Id> ids;
    ids.reserve(frameCallbacks.size());
    for (const auto &[id, callback] : frameCallbacks) ids.push_back(id);
    for (const auto id : ids) {
      const auto found = frameCallbacks.find(id);
      if (found == frameCallbacks.end()) continue;
      const auto callback = found->second;
      callback(now);
    }
    if (renderer) renderer();
  }

  // What a dropped task owns may post, or remove a timer, from its destructor:
  // everything goes while every container is still alive.
  void dropPending() {
    for (;;) {
      std::deque<Task> tasks;
      {
        std::lock_guard lock(mutex);
        tasks.swap(posted);
      }
      const auto droppedTimers = std::exchange(timers, {});
      const auto droppedCallbacks = std::exchange(frameCallbacks, {});
      if (tasks.empty() && droppedTimers.empty() && droppedCallbacks.empty()) break;
    }
    renderer = nullptr;
    eventHandler = nullptr;
    waitObserver = nullptr;
  }

  void pumpEvents(int timeoutMs) {
    SDL_Event event;
    waking.store(false);
    if (timeoutMs != 0) {
      if (waitObserver) waitObserver(true);
      const bool received = SDL_WaitEventTimeout(&event, timeoutMs);
      if (waitObserver) waitObserver(false);
      if (received) dispatch(event);
    }
    while (SDL_PollEvent(&event)) dispatch(event);
  }
};

MainLoop &MainLoop::get() {
  static MainLoop loop;
  return loop;
}

MainLoop::MainLoop() : state_(std::make_unique<State>()) {
  SDL_InitSubSystem(SDL_INIT_EVENTS);
}

MainLoop::~MainLoop() {
  state_->dropPending();
  SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

bool MainLoop::isMainThread() const {
  return std::this_thread::get_id() == state_->mainThread;
}

void MainLoop::post(Task task) {
  {
    std::lock_guard lock(state_->mutex);
    state_->posted.push_back(std::move(task));
  }
  state_->wake();
}

void MainLoop::quit(int exitCode) {
  state_->exitCode.store(exitCode);
  state_->quitting.store(true);
  state_->wake();
}

MainLoop::Id MainLoop::addTimer(Clock::duration delay, Task task) {
  const auto id = state_->nextId++;
  state_->timers.emplace(id, Timer{.due = Clock::now() + delay, .task = std::move(task)});
  return id;
}

void MainLoop::removeTimer(Id id) {
  state_->timers.erase(id);
}

MainLoop::Id MainLoop::addFrameCallback(FrameCallback callback) {
  const auto id = state_->nextId++;
  state_->frameCallbacks.emplace(id, std::move(callback));
  return id;
}

void MainLoop::removeFrameCallback(Id id) {
  state_->frameCallbacks.erase(id);
}

void MainLoop::requestFrame() {
  if (state_->frameRequested.exchange(true)) return;
  if (!isMainThread()) state_->wake();
}

void MainLoop::setRenderer(Renderer renderer) {
  state_->renderer = std::move(renderer);
}

void MainLoop::setEventHandler(EventHandler handler) {
  state_->eventHandler = std::move(handler);
}

void MainLoop::setWaitObserver(WaitObserver observer) {
  state_->waitObserver = std::move(observer);
}

MainLoop::Clock::duration MainLoop::framePeriod() const {
  return state_->framePeriod;
}

void MainLoop::setFramePeriod(Clock::duration period) {
  if (period > Clock::duration::zero()) state_->framePeriod = period;
}

void MainLoop::iterate(bool frame) {
  auto &state = *state_;
  state.pumpEvents(0);
  state.runPosted();
  state.runTimers();
  if (frame) state.runFrame(Clock::now());
}

int MainLoop::run() {
  auto &state = *state_;
  state.quitting.store(false);
  while (!state.quitting.load()) {
    const auto now = Clock::now();
    auto wakeAt = state.nextTimer();
    // A swap that waits for the display paces the frames by itself; where it
    // returns at once, as under Xvfb, the frame period does.
    if (state.wantsFrame()) wakeAt = std::min(wakeAt, state.nextFrame);
    bool hasPosted = false;
    {
      std::lock_guard lock(state.mutex);
      hasPosted = !state.posted.empty();
    }
    const int timeout = hasPosted ? 0 : wakeAt == Clock::time_point::max() ? -1 : millisecondsUntil(wakeAt, now);
    state.pumpEvents(timeout);
    state.runPosted();
    state.runTimers();
    if (state.wantsFrame() && Clock::now() >= state.nextFrame) state.runPacedFrame();
  }
  return state.exitCode.load();
}

} // namespace rnwebos
