#include "CrashReporter.h"

#include <doctest/doctest.h>

#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <array>
#include <bit>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

constexpr std::array kChainedSignals{SIGABRT, SIGSEGV, SIGBUS};

volatile std::sig_atomic_t chained = 0;
volatile std::sig_atomic_t chainedWithInfo = 0;

void recordChained(int signal) {
  chained = signal;
}

void recordChainedWithInfo(int signal, siginfo_t *, void *) {
  chainedWithInfo = signal;
}

void handle(int signal, void (*handler)(int)) {
  struct sigaction action{};
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  sigaction(signal, &action, nullptr);
}

void handleWithInfo(int signal, void (*handler)(int, siginfo_t *, void *)) {
  struct sigaction action{};
  action.sa_sigaction = handler;
  action.sa_flags = SA_SIGINFO;
  sigemptyset(&action.sa_mask);
  sigaction(signal, &action, nullptr);
}

class SavedActions final {
 public:
  SavedActions() {
    for (std::size_t index = 0; index < kChainedSignals.size(); ++index) {
      sigaction(kChainedSignals[index], nullptr, &actions_[index]);
    }
  }
  ~SavedActions() {
    for (std::size_t index = 0; index < kChainedSignals.size(); ++index) {
      sigaction(kChainedSignals[index], &actions_[index], nullptr);
    }
  }
  SavedActions(const SavedActions &) = delete;
  SavedActions &operator=(const SavedActions &) = delete;

 private:
  std::array<struct sigaction, kChainedSignals.size()> actions_{};
};

class Pipe final {
 public:
  Pipe() {
    REQUIRE(pipe(ends_.data()) == 0);
  }
  ~Pipe() {
    close(ends_[0]);
    if (ends_[1] >= 0) close(ends_[1]);
  }
  Pipe(const Pipe &) = delete;
  Pipe &operator=(const Pipe &) = delete;

  int writeEnd() const {
    return ends_[1];
  }

  std::string drain() {
    close(std::exchange(ends_[1], -1));
    std::string text;
    std::array<char, 4096> chunk{};
    for (auto length = read(ends_[0], chunk.data(), chunk.size()); length > 0;
         length = read(ends_[0], chunk.data(), chunk.size())) {
      text.append(chunk.data(), static_cast<std::size_t>(length));
    }
    return text;
  }

 private:
  std::array<int, 2> ends_{};
};

void pointAt(ucontext_t &context, const std::byte *instruction) {
  const auto address = std::bit_cast<std::uintptr_t>(instruction);
#if defined(__aarch64__)
  context.uc_mcontext.pc = address;
#elif defined(__x86_64__)
  context.uc_mcontext.gregs[REG_RIP] = static_cast<greg_t>(address);
#elif defined(__arm__)
  context.uc_mcontext.arm_pc = address;
#endif
}

std::size_t countOf(const std::string &text, const std::string &part) {
  std::size_t count = 0;
  for (auto at = text.find(part); at != std::string::npos; at = text.find(part, at + part.size())) ++count;
  return count;
}

} // namespace

TEST_CASE("installCrashReporter reports a crash in native code, then hands every signal to the handler before it") {
  const SavedActions saved;
  handle(SIGABRT, recordChained);
  handleWithInfo(SIGSEGV, recordChainedWithInfo);
  handle(SIGBUS, SIG_IGN);
  Pipe crashes;
  Pipe faultsOutsideLibraries;
  setCrashReportDescriptor(crashes.writeEnd());
  installCrashReporter();

  raise(SIGABRT);
  const auto afterAbort = chained;
  raise(SIGSEGV);
  const auto afterFault = chainedWithInfo;
  setCrashReportDescriptor(-1);
  raise(SIGBUS);
  const auto reports = crashes.drain();
  struct sigaction installed{};
  sigaction(SIGSEGV, nullptr, &installed);
  siginfo_t info{};
  ucontext_t withoutInstruction{};
  ucontext_t inCompiledCode{};
  const std::vector<std::byte> compiledCode(16);
  pointAt(inCompiledCode, compiledCode.data());
  chainedWithInfo = 0;
  setCrashReportDescriptor(faultsOutsideLibraries.writeEnd());
  installed.sa_sigaction(SIGSEGV, &info, &withoutInstruction);
  const auto afterFaultWithoutInstruction = chainedWithInfo;
  chainedWithInfo = 0;
  installed.sa_sigaction(SIGSEGV, &info, &inCompiledCode);
  const auto afterFaultInCompiledCode = chainedWithInfo;
  setCrashReportDescriptor(-1);
  const auto outsideReports = faultsOutsideLibraries.drain();

  CHECK(afterAbort == SIGABRT);
  CHECK(afterFault == SIGSEGV);
  CHECK(countOf(reports, "Native crash: ") == 2);
  CHECK(reports.find("Native crash: SIGABRT at 0x") != std::string::npos);
  CHECK(reports.find("Native crash: SIGSEGV at 0x") != std::string::npos);
  CHECK(reports.find("\\n#0 ") != std::string::npos);
  CHECK(afterFaultWithoutInstruction == SIGSEGV);
  CHECK(afterFaultInCompiledCode == SIGSEGV);
  CHECK(outsideReports.empty());
}

} // namespace rntizen
