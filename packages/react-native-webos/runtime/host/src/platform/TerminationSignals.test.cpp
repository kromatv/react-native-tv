#include "TerminationSignals.h"

#include <doctest/doctest.h>

#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <csignal>

namespace rnwebos {

namespace {

constexpr int kHandled = 42;

/// Runs `watch` in a child process, sends it `signals` SIGTERMs once it is ready, and returns how it exited.
int exitStatusAfterSigterm(void (*watch)(), int signals) {
  std::array<int, 2> ready{};
  REQUIRE(pipe(ready.data()) == 0);
  const pid_t child = fork();
  REQUIRE(child >= 0);
  if (child == 0) {
    blockTerminationSignals();
    watch();
    const char byte = 1;
    if (write(ready[1], &byte, 1) != 1) _exit(1);
    for (;;) pause();
  }
  char byte = 0;
  REQUIRE(read(ready[0], &byte, 1) == 1);
  for (int sent = 0; sent < signals; ++sent) {
    kill(child, SIGTERM);
    usleep(50'000);
  }
  int status = 0;
  REQUIRE(waitpid(child, &status, 0) == child);
  close(ready[0]);
  close(ready[1]);
  return WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
}

} // namespace

TEST_CASE("SIGTERM reaches the watcher instead of ending the process") {
  CHECK(exitStatusAfterSigterm([] { watchTerminationSignals([] { _exit(kHandled); }); }, 1) == kHandled);
}

TEST_CASE("a second SIGTERM ends a process that did not leave after the first") {
  CHECK(exitStatusAfterSigterm([] { watchTerminationSignals([] {}); }, 2) == 128 + SIGTERM);
}

} // namespace rnwebos
