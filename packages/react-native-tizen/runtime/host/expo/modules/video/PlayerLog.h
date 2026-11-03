#pragma once

#include "logging/Log.h"

#include <player.h>

#include <chrono>
#include <string>

namespace rntizen::expo {

/// Logs `message` for the player numbered `number`, as `[video 2] message`.
void playerLog(int number, const std::string &message, LogLevel level = LogLevel::Info);

/// The name of a `player_error_e`, such as `PLAYER_ERROR_INVALID_URI`.
std::string playerErrorName(int code);

const char *playerStateName(player_state_e state);

/// `value` with `digits` decimals and a point, whatever the TV's locale.
std::string decimal(double value, int digits = 3);

/// The milliseconds since `start`, with one decimal, as `12.5 ms`.
std::string millisSince(std::chrono::steady_clock::time_point start);

/// "on the main loop" or "off the main loop", with the calling thread's id.
std::string threadDescription();

} // namespace rntizen::expo
