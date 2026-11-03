#pragma once

#include <optional>
#include <string>

namespace rntizen::expo {

/// A random (version 4) UUID in lowercase, as `expo.uuidv4()` returns it.
std::string uuidV4();

/// The name-based (version 5, SHA-1) UUID of `name` in `namespaceUuid`, as
/// RFC 4122 section 4.3 defines it; nullopt when `namespaceUuid` is not a UUID.
std::optional<std::string> uuidV5(const std::string &name, const std::string &namespaceUuid);

} // namespace rntizen::expo
