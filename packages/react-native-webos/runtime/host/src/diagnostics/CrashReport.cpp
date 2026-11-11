#include "CrashReport.h"

#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>

#include <bit>
#include <cstring>

namespace rntizen {

namespace {

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

} // namespace

CrashReport::CrashReport(int signal, std::uintptr_t faultAddress) {
  append("\n{\"level\":\"error\",\"message\":\"Native crash: ");
  append(signalName(signal));
  append(" at 0x");
  appendHex(faultAddress);
}

void CrashReport::addFrame(int index, std::uintptr_t address) {
  append("\\n#");
  appendDecimal(index);
  append(" ");
  Dl_info info{};
  if (dladdr(std::bit_cast<const void *>(address), &info) != 0 && info.dli_fname != nullptr) {
    const char *slash = std::strrchr(info.dli_fname, '/');
    append(slash != nullptr ? slash + 1 : info.dli_fname);
    append("+0x");
    appendHex(address - std::bit_cast<std::uintptr_t>(info.dli_fbase));
  } else {
    append("0x");
    appendHex(address);
  }
}

void CrashReport::close() {
  append("\"}\n");
}

std::string_view CrashReport::text() const {
  return {data_.data(), size_};
}

void CrashReport::writeTo(int descriptor) const {
  std::size_t written = 0;
  while (written < size_) {
    const auto result = write(descriptor, data_.data() + written, size_ - written);
    if (result <= 0) return;
    written += static_cast<std::size_t>(result);
  }
}

void CrashReport::append(std::string_view text) {
  for (const char character : text) {
    if (size_ == data_.size()) return;
    data_[size_++] = character;
  }
}

void CrashReport::appendHex(std::uintptr_t value) {
  std::array<char, 2 * sizeof(value)> digits{};
  std::size_t count = 0;
  do {
    digits[count++] = "0123456789abcdef"[value & 0xf];
    value >>= 4;
  } while (value != 0);
  while (count > 0) append({&digits[--count], 1});
}

void CrashReport::appendDecimal(int value) {
  std::array<char, 12> digits{};
  std::size_t count = 0;
  do {
    digits[count++] = static_cast<char>('0' + value % 10);
    value /= 10;
  } while (value != 0);
  while (count > 0) append({&digits[--count], 1});
}

} // namespace rntizen
