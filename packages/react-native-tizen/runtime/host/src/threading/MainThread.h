#pragma once

#include <chrono>
#include <functional>

namespace rntizen {

/// Runs `task` on the EFL main loop, the only thread allowed to touch Evas.
/// From the main thread itself it still queues, so a caller never re-enters.
/// What `task` throws is logged: an exception must not unwind through Ecore.
void runOnMainThread(std::function<void()> task);

/// Runs `task` on the main loop and waits for it, rethrowing what it throws;
/// runs inline on the main thread. Returns false without running it once the
/// main loop is closing, so a worker can never wait on a loop that is busy
/// tearing that worker down.
bool runOnMainThreadSync(const std::function<void()> &task);

/// Runs `task` on the main loop and waits for it, as runOnMainThreadSync does,
/// but ahead of the work queued before it: the main loop also runs it at its
/// next `runPriorityTasks()`, such as between two mutations of a long mount.
bool runOnMainThreadSoon(const std::function<void()> &task);

/// Runs the tasks waiting in runOnMainThreadSoon. Main thread only; with none
/// waiting it costs one atomic load, so a long loop may call it every step.
void runPriorityTasks();

/// Releases every runOnMainThreadSync waiter and refuses new ones. Call on the
/// main thread before joining a thread that may be waiting on the main loop.
void closeMainThreadSync();

/// Accepts runOnMainThreadSync calls again, for a host started after another closed it.
void openMainThreadSync();

bool isMainThread();

/// Starts or stops adding up how long workers wait for the main loop.
void setMainThreadWaitMeasured(bool measured);

/// How long workers have waited in runOnMainThreadSync and runOnMainThreadSoon
/// while measured, in total, the waits still going on so far included.
std::chrono::nanoseconds mainThreadWaitTime();

} // namespace rntizen
