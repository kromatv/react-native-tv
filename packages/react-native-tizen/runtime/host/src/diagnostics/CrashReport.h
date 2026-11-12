#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace rntizen {

/// A native crash report, built as a signal handler must build it, without
/// allocating or locking: one log stream JSON line,
/// `{"level":"error","message":"Native crash: SIGSEGV at 0x..."}`, whose
/// message has a `#N library+0xoffset` line per frame. Text past 8 KiB is cut.
class CrashReport final {
 public:
  CrashReport(int signal, std::uintptr_t faultAddress);

  /// Adds frame `index`: the library holding `address` and the offset in it,
  /// or the bare address when no loaded library holds it.
  void addFrame(int index, std::uintptr_t address);
  /// Ends the JSON line; add no frame after it.
  void close();

  std::string_view text() const;
  /// Writes the report to `descriptor`, stopping at the first failed write.
  void writeTo(int descriptor) const;

 private:
  void append(std::string_view text);
  void appendHex(std::uintptr_t value);
  void appendDecimal(int value);

  std::array<char, 8192> data_{};
  std::size_t size_{0};
};

} // namespace rntizen
