// std::random_device, which Hermes builds for every runtime and React Native
// and the host draw identifiers from. The one in the SDK's static libstdc++
// opens /dev/urandom and throws when it cannot, and an app's jail may not hold
// that file (docs/reference/webos-native-toolchain.md, section 4). These
// definitions keep its object out of the link and ask getrandom instead; if
// libstdc++'s were linked too, the link would fail on the duplicates.

#include <cerrno>
#include <cstddef>
#include <random>

#include <sys/random.h>

namespace std {

// libstdc++'s own string instantiations call this overload, so it is defined
// too: left to libstdc++, it would bring the whole object back.
void random_device::_M_init(const char *, size_t) {
  _M_file = nullptr;
  _M_func = nullptr;
  _M_fd = -1;
}

void random_device::_M_init(const std::string &token) {
  _M_init(token.data(), token.size());
}

void random_device::_M_fini() {
  // Nothing to close: _M_init opens no file.
}

random_device::result_type random_device::_M_getval() {
  result_type value;
  auto *bytes = reinterpret_cast<std::byte *>(&value);
  size_t filled = 0;
  while (filled < sizeof value) {
    const auto received = getrandom(bytes + filled, sizeof value - filled, 0);
    if (received > 0) {
      filled += static_cast<size_t>(received);
    } else if (received == 0 || errno != EINTR) {
      __throw_runtime_error("random_device: the kernel gave no random bytes");
    }
  }
  return value;
}

double random_device::_M_getentropy() const noexcept {
  return 32.0;
}

} // namespace std
