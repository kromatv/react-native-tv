#pragma once

#include "FrameStats.h"

#include <ReactCommon/RuntimeExecutor.h>

#include <Ecore.h>
#include <Evas.h>

#include <chrono>
#include <memory>

namespace rntizen {

class ControlLink;
class TaskQueueThread;

/// What `rn-tizen perf` prints. While the listener on a development build's
/// control link asks for it, logs one `perf {...}` line a second: the main
/// loop's frames (Evas renders and their time, display ticks, dropped frames,
/// busy time), the JavaScript thread's busy time, its wait on the main loop and
/// its queue delay, Fabric's commits and mounts, the Hermes heap and the
/// process's CPU. Otherwise it measures nothing. Create, use and destroy it on
/// the main thread.
class PerfMonitor final {
 public:
  struct Sources {
    Evas *evas;
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

  static Eina_Bool onCheck(void *data);
  static Eina_Bool onTick(void *data);
  static Eina_Bool onIdleEnter(void *data);
  static Eina_Bool onIdleExit(void *data);
  static void onRenderPre(void *data, Evas *evas, void *info);
  static void onRenderPost(void *data, Evas *evas, void *info);

  void start();
  void stop();
  void emit();
  void probeJavaScript();

  const Sources sources_;
  const std::chrono::steady_clock::time_point createdAt_{std::chrono::steady_clock::now()};
  const std::shared_ptr<JsProbe> probe_;
  Ecore_Timer *check_{nullptr};
  Ecore_Animator *animator_{nullptr};
  Ecore_Idle_Enterer *idleEnterer_{nullptr};
  Ecore_Idle_Exiter *idleExiter_{nullptr};
  std::unique_ptr<FrameStats> frames_;
  std::chrono::steady_clock::time_point windowStart_;
  std::chrono::nanoseconds jsBusyAt_{0};
  std::chrono::nanoseconds jsWaitAt_{0};
  double cpuSecondsAt_{0};
};

} // namespace rntizen
