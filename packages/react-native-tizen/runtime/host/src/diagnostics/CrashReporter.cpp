#include "CrashReporter.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace rntizen {

namespace {

constexpr std::array kSignals{SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
constexpr int kMaxFrames = 48;
constexpr std::size_t kAlternateStackSize = 64 * 1024;

std::atomic<int> reportDescriptor{-1};
std::array<struct sigaction, kSignals.size()> previousActions{};

class ReportBuffer {
 public:
  void append(const char *text) {
    while (*text != '\0' && size_ < sizeof(data_)) data_[size_++] = *text++;
  }

  void appendHex(std::uintptr_t value) {
    char digits[2 * sizeof(value)];
    int count = 0;
    do {
      digits[count++] = "0123456789abcdef"[value & 0xf];
      value >>= 4;
    } while (value != 0);
    while (count > 0 && size_ < sizeof(data_)) data_[size_++] = digits[--count];
  }

  void appendDecimal(int value) {
    char digits[12];
    int count = 0;
    do {
      digits[count++] = static_cast<char>('0' + value % 10);
      value /= 10;
    } while (value != 0);
    while (count > 0 && size_ < sizeof(data_)) data_[size_++] = digits[--count];
  }

  void writeTo(int descriptor) const {
    std::size_t written = 0;
    while (written < size_) {
      const auto result = write(descriptor, data_ + written, size_ - written);
      if (result <= 0) return;
      written += static_cast<std::size_t>(result);
    }
  }

 private:
  char data_[8192];
  std::size_t size_{0};
};

const char *signalName(int signal) {
  switch (signal) {
    case SIGSEGV:
      return "SIGSEGV";
    case SIGBUS:
      return "SIGBUS";
    case SIGILL:
      return "SIGILL";
    case SIGFPE:
      return "SIGFPE";
    case SIGABRT:
      return "SIGABRT";
    default:
      return "signal";
  }
}

std::uintptr_t faultingInstruction(const void *context) {
  const auto &machine = static_cast<const ucontext_t *>(context)->uc_mcontext;
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
  return address != 0 && dladdr(reinterpret_cast<void *>(address), &info) != 0 && info.dli_fname != nullptr;
}

void appendFrame(ReportBuffer &report, int index, void *frame) {
  report.append("\\n#");
  report.appendDecimal(index);
  report.append(" ");
  Dl_info info{};
  if (dladdr(frame, &info) != 0 && info.dli_fname != nullptr) {
    const char *slash = std::strrchr(info.dli_fname, '/');
    report.append(slash != nullptr ? slash + 1 : info.dli_fname);
    report.append("+0x");
    report.appendHex(reinterpret_cast<std::uintptr_t>(frame) - reinterpret_cast<std::uintptr_t>(info.dli_fbase));
  } else {
    report.append("0x");
    report.appendHex(reinterpret_cast<std::uintptr_t>(frame));
  }
}

void report(int signal, const siginfo_t *info) {
  std::array<void *, kMaxFrames> frames{};
  const int count = backtrace(frames.data(), kMaxFrames);
  ReportBuffer buffer;
  buffer.append("\n{\"level\":\"error\",\"message\":\"Native crash: ");
  buffer.append(signalName(signal));
  buffer.append(" at 0x");
  buffer.appendHex(reinterpret_cast<std::uintptr_t>(info->si_addr));
  for (int index = 0; index < count; ++index) appendFrame(buffer, index, frames[static_cast<std::size_t>(index)]);
  buffer.append("\"}\n");
  buffer.writeTo(STDERR_FILENO);
  if (const int descriptor = reportDescriptor.load(); descriptor >= 0) buffer.writeTo(descriptor);
}

void chainToPrevious(int signal, siginfo_t *info, void *context) {
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
  if (signal == SIGABRT || isInNativeLibrary(faultingInstruction(context))) report(signal, info);
  chainToPrevious(signal, info, context);
}

} // namespace

void installCrashReporter() {
  static std::once_flag once;
  std::call_once(once, [] {
    std::array<void *, 1> warmUp{};
    backtrace(warmUp.data(), 1);
    stack_t alternateStack{};
    alternateStack.ss_sp = std::malloc(kAlternateStackSize);
    alternateStack.ss_size = kAlternateStackSize;
    sigaltstack(&alternateStack, nullptr);
    struct sigaction action{};
    action.sa_sigaction = onCrash;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    for (std::size_t index = 0; index < kSignals.size(); ++index) {
      sigaction(kSignals[index], &action, &previousActions[index]);
    }
  });
}

void setCrashReportDescriptor(int descriptor) {
  reportDescriptor.store(descriptor);
}

} // namespace rntizen
