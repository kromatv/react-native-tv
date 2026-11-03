#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace rntizen {

/// MD5 of `data`, as 32 lowercase hexadecimal digits: how Expo names cached
/// assets and what `File.md5` reports.
std::string md5Hex(std::string_view data);

/// SHA-1 of `data`: name-based (version 5) UUIDs and the WebSocket handshake.
std::array<std::uint8_t, 20> sha1(std::string_view data);

} // namespace rntizen
