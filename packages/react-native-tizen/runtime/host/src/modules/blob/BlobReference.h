#pragma once

#include <jsi/jsi.h>

#include <cstddef>
#include <optional>
#include <string>

namespace rntizen {

/// The `BlobData` JavaScript passes for a blob: `{blobId, offset, size, type}`
/// (`react-native/Libraries/Blob/BlobTypes.js`).
struct BlobReference {
  std::string blobId;
  std::size_t offset;
  std::optional<std::size_t> size;
  std::string type;
};

/// Throws a JSError when `data` carries no `blobId`.
BlobReference readBlobReference(facebook::jsi::Runtime &runtime, const facebook::jsi::Object &data);

/// The bytes `reference` names, or nullopt when the store has no such range.
std::optional<std::string> resolveBlob(const BlobReference &reference);

} // namespace rntizen
