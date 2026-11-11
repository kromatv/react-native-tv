#pragma once

#include "../platform/MainLoop.h"
#include "FrameStats.h"

#include <ReactCommon/RuntimeExecutor.h>

#include <chrono>
#include <memory>

namespace rnwebos {

class ControlLink;
class Scene;
class TaskQueueThread;
struct FrameTiming;

/// What `rn-webos perf` prints. While the listener on a development build's
/// control link asks for it, logs one `perf {...}` line a second: the main
/// loop's frames (the scene's renders and their time, display ticks, dropped
/// frames, busy time), the JavaScript thread's busy time, its wait on the main
/// loop and its queue delay, Fabric's commits and mounts, the Hermes heap and
/// the process's CPU. Otherwise it measures nothing. Create, use and destroy
/// it on the main thread.
class PerfMonitor final {
 public:
  struct Sources {
    Scene *scene;
    const ControlLink *controlLink;
    std::shared_ptr<TaskQueueThread> jsThread;
    facebook::react::RuntimeExecutor runtimeExecutor;
  };

  explicit PerfMonitor(Sources sources);
  ~PerfMonitor();

  PerfMonitor(const PerfMonitor &) = delete;
  PerfMonitor &operator=(const PerfMonitor &) = delete;

 private:
  struct JsProbe;

  void check();
  void start();
  void stop();
  void emit();
  void probeJavaScript();
  void frameDrawn(const FrameTiming &timing);

  const Sources sources_;
  const std::chrono::steady_clock::time_point createdAt_{std::chrono::steady_clock::now()};
  const std::shared_ptr<JsProbe> probe_;
  MainLoop::Id check_{0};
  MainLoop::Id frameCallback_{0};
  std::unique_ptr<FrameStats> frames_;
  std::chrono::steady_clock::time_point windowStart_;
  std::chrono::nanoseconds jsBusyAt_{0};
  std::chrono::nanoseconds jsWaitAt_{0};
  double cpuSecondsAt_{0};
};

} // namespace rnwebos
