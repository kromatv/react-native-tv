#pragma once

#include <Ecore.h>

#include <functional>

namespace rntizen {

/// Calls back once before each frame the main loop draws, for as long as it
/// runs. An Ecore animator alone calls back at every tick, and a TV delivers
/// several ticks in one turn of the loop once a frame takes longer than the
/// display's period (efl-rendering.md section 1): every step but the last
/// would move views no frame shows. Main thread only.
class FrameTicker final {
 public:
  FrameTicker() = default;
  ~FrameTicker();

  FrameTicker(const FrameTicker &) = delete;
  FrameTicker &operator=(const FrameTicker &) = delete;

  /// Starts calling `onFrame`, or replaces what a running ticker calls.
  void start(std::function<void()> onFrame);
  void stop();
  bool isRunning() const;

 private:
  static Eina_Bool onTick(void *data);
  static Eina_Bool onBeforeRender(void *data);

  std::function<void()> onFrame_;
  Ecore_Animator *animator_{nullptr};
  Ecore_Idle_Enterer *beforeRender_{nullptr};
  bool ticked_{false};
};

} // namespace rntizen
