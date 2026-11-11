#pragma once

#include <SDL3/SDL_events.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace rnwebos {

/// The app's main loop: SDL's events, tasks posted from any thread, timers,
/// and one tick per display frame while something asks for frames. The thread
/// that first calls `get` is the main thread, the only one that may touch the
/// window, the scene and the views.
class MainLoop final {
 public:
  using Clock = std::chrono::steady_clock;
  using Task = std::function<void()>;
  using Id = std::uint64_t;
  /// Runs once per frame, before the frame is drawn.
  using FrameCallback = std::function<void(Clock::time_point frameTime)>;
  /// Draws the window when it changed. True when it presented a frame.
  using Renderer = std::function<bool()>;
  using EventHandler = std::function<void(const SDL_Event &event)>;
  /// Told when the loop starts waiting for something to do (true) and when it
  /// wakes (false): what `rn-webos perf` adds the loop's load up from.
  using WaitObserver = std::function<void(bool waiting)>;

  static MainLoop &get();

  MainLoop(const MainLoop &) = delete;
  MainLoop &operator=(const MainLoop &) = delete;

  /// Runs until `quit`, and returns its exit code. Main thread only.
  int run();
  /// Ends `run` once the current iteration is done. Any thread.
  void quit(int exitCode = 0);
  bool isMainThread() const;

  /// Runs `task` on the main thread in a later iteration, never inline, even
  /// when called on the main thread. Any thread. What it throws is the task's
  /// to catch: an exception here ends the app.
  void post(Task task);

  /// Runs `task` once, `delay` from now. Main thread only.
  Id addTimer(Clock::duration delay, Task task);
  void removeTimer(Id id);

  /// Calls `callback` every display frame until it is removed. Main thread only.
  Id addFrameCallback(FrameCallback callback);
  void removeFrameCallback(Id id);

  /// Asks for one frame: the renderer runs at the next frame time. Any thread.
  void requestFrame();

  void setRenderer(Renderer renderer);
  void setEventHandler(EventHandler handler);
  void setWaitObserver(WaitObserver observer);

  /// Runs what is due without waiting: events, posted tasks, timers. With
  /// `frame`, also one frame's callbacks and the renderer. For tests, and for
  /// code that must let the loop breathe during a long main-thread job.
  void iterate(bool frame = false);

  /// The display's frame period: 60 Hz unless the window reported another rate.
  Clock::duration framePeriod() const;
  void setFramePeriod(Clock::duration period);

 private:
  MainLoop();
  ~MainLoop();

  struct State;
  const std::unique_ptr<State> state_;
};

} // namespace rnwebos
