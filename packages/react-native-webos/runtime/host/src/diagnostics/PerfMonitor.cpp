#include "PerfMonitor.h"

#include "../logging/Log.h"
#include "../render/Scene.h"
#include "../threading/MainThread.h"
#include "../threading/TaskQueueThread.h"
#include "ControlLink.h"
#include "PerfSample.h"
#include "TransactionStats.h"

#include <jsi/instrumentation.h>
#include <jsi/jsi.h>

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace rnwebos {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto kCheckInterval = std::chrono::milliseconds(250);
constexpr auto kWindow = std::chrono::seconds(1);
constexpr double kBytesPerMb = 1024.0 * 1024.0;

template <typename Duration>
double millis(Duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

double processCpuSeconds() {
  std::FILE *file = std::fopen("/proc/self/stat", "r");
  if (file == nullptr) return 0;
  std::array<char, 1024> buffer{};
  const std::size_t length = std::fread(buffer.data(), 1, buffer.size() - 1, file);
  std::fclose(file);
  buffer[length] = '\0';
  const char *afterName = std::strrchr(buffer.data(), ')');
  if (afterName == nullptr) return 0;
  unsigned long user = 0;
  unsigned long system = 0;
  if (std::sscanf(afterName + 1, " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", &user, &system) != 2) {
    return 0;
  }
  return static_cast<double>(user + system) / static_cast<double>(sysconf(_SC_CLK_TCK));
}

double heapValue(const std::unordered_map<std::string, std::int64_t> &info, const char *key) {
  const auto found = info.find(key);
  return found == info.end() ? 0 : static_cast<double>(found->second);
}

} // namespace

struct PerfMonitor::JsProbe {
  std::mutex mutex;
  bool pending{false};
  Clock::time_point postedAt;
  double lagMs{0};
  HeapReading heap;
};

PerfMonitor::PerfMonitor(Sources sources)
    : sources_(std::move(sources)), probe_(std::make_shared<JsProbe>()) {
  check();
}

PerfMonitor::~PerfMonitor() {
  stop();
  MainLoop::get().removeTimer(check_);
}

void PerfMonitor::check() {
  check_ = MainLoop::get().addTimer(kCheckInterval, [this] { check(); });
  const bool wanted = sources_.controlLink != nullptr && sources_.controlLink->perfRequested();
  if (wanted && !frames_) start();
  if (!wanted && frames_) stop();
  if (frames_ && Clock::now() - windowStart_ >= kWindow) emit();
}

void PerfMonitor::start() {
  const auto now = Clock::now();
  auto &loop = MainLoop::get();
  frames_ = std::make_unique<FrameStats>(millis(loop.framePeriod()));
  frames_->woke(now);
  frameCallback_ = loop.addFrameCallback([this](Clock::time_point frameTime) { frames_->tick(frameTime); });
  loop.setWaitObserver([this](bool waiting) {
    if (waiting) {
      frames_->sleeping(Clock::now());
    } else {
      frames_->woke(Clock::now());
    }
  });
  sources_.scene->setFrameObserver([this](const FrameTiming &timing) { frameDrawn(timing); });
  sources_.jsThread->setMeasured(true);
  setMainThreadWaitMeasured(true);
  setTransactionStatsEnabled(true);
  takeTransactionStats();
  windowStart_ = now;
  jsBusyAt_ = sources_.jsThread->busyTime();
  jsWaitAt_ = mainThreadWaitTime();
  cpuSecondsAt_ = processCpuSeconds();
  probeJavaScript();
  log(LogLevel::Debug, "Perf samples on");
}

void PerfMonitor::stop() {
  if (!frames_) return;
  auto &loop = MainLoop::get();
  sources_.scene->setFrameObserver(nullptr);
  loop.setWaitObserver(nullptr);
  loop.removeFrameCallback(std::exchange(frameCallback_, 0));
  sources_.jsThread->setMeasured(false);
  setMainThreadWaitMeasured(false);
  setTransactionStatsEnabled(false);
  frames_.reset();
  log(LogLevel::Debug, "Perf samples off");
}

// A frame is the scene's draw and its flush to the GPU. The swap after them
// waits for the display where the display paces it, which is the loop resting.
void PerfMonitor::frameDrawn(const FrameTiming &timing) {
  const auto flushed = timing.started + timing.draw + timing.flush;
  frames_->renderStarted(timing.started);
  frames_->renderFinished(flushed);
  frames_->sleeping(flushed);
  frames_->woke(flushed + timing.present);
}

void PerfMonitor::emit() {
  const auto now = Clock::now();
  PerfSample sample;
  sample.uptimeMs = millis(now - createdAt_);
  sample.windowMs = millis(now - windowStart_);
  sample.frames = frames_->take(now);
  const auto jsBusy = sources_.jsThread->busyTime();
  const auto jsWait = mainThreadWaitTime();
  sample.jsBusyMs = millis(std::max(jsBusy - jsBusyAt_, std::chrono::nanoseconds(0)));
  sample.jsWaitMs = millis(std::max(jsWait - jsWaitAt_, std::chrono::nanoseconds(0)));
  jsBusyAt_ = std::max(jsBusyAt_, jsBusy);
  jsWaitAt_ = std::max(jsWaitAt_, jsWait);
  sample.transactions = takeTransactionStats();
  {
    std::lock_guard lock(probe_->mutex);
    sample.jsLagMs = probe_->pending ? millis(now - probe_->postedAt) : probe_->lagMs;
    sample.heap = probe_->heap;
  }
  sample.memory = readProcessMemory();
  const double cpuSeconds = processCpuSeconds();
  if (sample.windowMs > 0) sample.cpuPercent = (cpuSeconds - cpuSecondsAt_) * 100000 / sample.windowMs;
  cpuSecondsAt_ = cpuSeconds;
  windowStart_ = now;
  log(LogLevel::Debug, perfLine(sample));
  probeJavaScript();
}

void PerfMonitor::probeJavaScript() {
  {
    std::lock_guard lock(probe_->mutex);
    if (probe_->pending) return;
    probe_->pending = true;
    probe_->postedAt = Clock::now();
  }
  sources_.runtimeExecutor([probe = probe_](facebook::jsi::Runtime &runtime) {
    const auto info = runtime.instrumentation().getHeapInfo(false);
    std::lock_guard lock(probe->mutex);
    probe->pending = false;
    probe->lagMs = millis(Clock::now() - probe->postedAt);
    probe->heap = HeapReading{
        .usedMb = heapValue(info, "hermes_allocatedBytes") / kBytesPerMb,
        .sizeMb = heapValue(info, "hermes_heapSize") / kBytesPerMb,
        .collections = static_cast<std::int64_t>(heapValue(info, "hermes_numCollections")),
    };
  });
}

} // namespace rnwebos
