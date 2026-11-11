#include "CrashReporter.h"

#include "CrashReport.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace rntizen {

namespace {

constexpr std::array kSignals{SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
constexpr int kMaxFrames = 48;
constexpr std::size_t kAlternateStackSize = 64 * 1024;

struct HandlerState {
  std::atomic<int> reportDescriptor{-1};
  std::array<struct sigaction, kSignals.size()> previousActions{};
};

HandlerState &handlerState() {
  static constinit HandlerState state;
  return state;
}

std::uintptr_t faultingInstruction(const ucontext_t &context) {
  const auto &machine = context.uc_mcontext;
#if defined(__arm__)
  return machine.arm_pc;
#elif defined(__aarch64__)
  return machine.pc;
#elif defined(__x86_64__)
  return static_cast<std::uintptr_t>(machine.gregs[REG_RIP]);
#else
  return 0;
#endif
}

bool isInNativeLibrary(std::uintptr_t address) {
  Dl_info info{};
  return address != 0 && dladdr(std::bit_cast<const void *>(address), &info) != 0 && info.dli_fname != nullptr;
}

void report(int signal, const siginfo_t &info) {
  std::array<void *, kMaxFrames> frames{};
  const int count = backtrace(frames.data(), kMaxFrames);
  CrashReport crash(signal, std::bit_cast<std::uintptr_t>(info.si_addr));
  for (int index = 0; index < count; ++index) {
    crash.addFrame(index, std::bit_cast<std::uintptr_t>(frames[static_cast<std::size_t>(index)]));
  }
  crash.close();
  crash.writeTo(STDERR_FILENO);
  if (const int descriptor = handlerState().reportDescriptor.load(); descriptor >= 0) crash.writeTo(descriptor);
}

void chainToPrevious(int signal, siginfo_t *info, void *context) {
  const auto &previousActions = handlerState().previousActions;
  for (std::size_t index = 0; index < kSignals.size(); ++index) {
    if (kSignals[index] != signal) continue;
    const auto &previous = previousActions[index];
    if ((previous.sa_flags & SA_SIGINFO) != 0 && previous.sa_sigaction != nullptr) {
      previous.sa_sigaction(signal, info, context);
      return;
    }
    if (previous.sa_handler == SIG_IGN) return;
    if (previous.sa_handler != SIG_DFL && previous.sa_handler != nullptr) {
      previous.sa_handler(signal);
      return;
    }
  }
  struct sigaction fallback{};
  fallback.sa_handler = SIG_DFL;
  sigaction(signal, &fallback, nullptr);
  raise(signal);
}

void onCrash(int signal, siginfo_t *info, void *context) {
  if (signal == SIGABRT || isInNativeLibrary(faultingInstruction(*static_cast<const ucontext_t *>(context)))) {
    report(signal, *info);
  }
  chainToPrevious(signal, info, context);
}

} // namespace

void installCrashReporter() {
  static std::once_flag once;
  std::call_once(once, [] {
    std::array<void *, 1> warmUp{};
    backtrace(warmUp.data(), 1);
    alignas(16) static std::array<std::byte, kAlternateStackSize> alternateStackMemory{};
    stack_t alternateStack{};
    alternateStack.ss_sp = alternateStackMemory.data();
    alternateStack.ss_size = kAlternateStackSize;
    sigaltstack(&alternateStack, nullptr);
    struct sigaction action{};
    action.sa_sigaction = onCrash;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    auto &previousActions = handlerState().previousActions;
    for (std::size_t index = 0; index < kSignals.size(); ++index) {
      sigaction(kSignals[index], &action, &previousActions[index]);
    }
  });
}

void setCrashReportDescriptor(int descriptor) {
  handlerState().reportDescriptor.store(descriptor);
}

} // namespace rntizen
