#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace rntizen {

/// Connects to `host` (an IPv4 address) on `port`, giving up after `timeout`
/// or once `cancelled` returns true, which it asks every 250 ms. Returns a
/// blocking socket, or -1.
int connectTcp(
    const std::string &host,
    std::uint16_t port,
    std::chrono::milliseconds timeout,
    const std::function<bool()> &cancelled);

/// Whether the other end closed `socket`, on a connection it never writes to:
/// anything to read is its end of stream or an error. Never blocks.
bool peerHasClosed(int socket);

/// Writes all of `data`, retrying after a signal. False once the connection failed.
bool sendAll(int socket, const void *data, std::size_t size);

} // namespace rntizen
