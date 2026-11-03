#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace rntizen {

/// The bytes behind React Native's `Blob`s, by blob id: what `BlobModule`,
/// `FileReaderModule`, `Networking`, `WebSocketModule` and Expo's fetch hand
/// each other. One per process, as a TV app runs one React Native instance.
/// Thread-safe.
class BlobStore final {
 public:
  static BlobStore &shared();

  /// Keeps `bytes` under a new random id and returns the id.
  std::string store(std::string bytes);
  void store(const std::string &blobId, std::string bytes);

  /// `size` bytes from `offset` (to the end when `size` is absent), or
  /// nullopt for an unknown id or a range outside the blob.
  std::optional<std::string> resolve(
      const std::string &blobId,
      std::size_t offset,
      std::optional<std::size_t> size) const;

  std::size_t size(const std::string &blobId) const;
  void remove(const std::string &blobId);

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::shared_ptr<const std::string>> blobs_;
};

} // namespace rntizen
