#include "TerminationSignals.h"

#include <pthread.h>
#include <unistd.h>

#include <csignal>
#include <thread>
#include <utility>

namespace rnwebos {

namespace {

sigset_t terminationSignals() {
  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGTERM);
  sigaddset(&signals, SIGINT);
  return signals;
}

} // namespace

void blockTerminationSignals() {
  const auto signals = terminationSignals();
  pthread_sigmask(SIG_BLOCK, &signals, nullptr);
}

void watchTerminationSignals(std::function<void()> onTerminate) {
  std::thread([onTerminate = std::move(onTerminate)] {
    const auto signals = terminationSignals();
    int signal = 0;
    if (sigwait(&signals, &signal) != 0) return;
    onTerminate();
    if (sigwait(&signals, &signal) == 0) _exit(128 + signal);
  }).detach();
}

} // namespace rnwebos
