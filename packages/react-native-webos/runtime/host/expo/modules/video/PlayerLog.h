#pragma once

#include "logging/Log.h"

#include <string>

namespace rnwebos::expo {

/// Logs `message` for the player numbered `number`, as `[video 2] message`.
void playerLog(int number, const std::string &message, LogLevel level = LogLevel::Info);

/// `value` with `digits` decimals and a point, whatever the TV's locale.
std::string decimal(double value, int digits = 3);

} // namespace rnwebos::expo
