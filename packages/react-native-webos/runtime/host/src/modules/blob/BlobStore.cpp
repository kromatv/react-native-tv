#include "BlobStore.h"

#include <cstdint>
#include <format>
#include <random>

namespace rntizen {

namespace {

std::uint64_t randomWord(std::random_device &device) {
  const std::uint64_t high = device();
  return (high << 32) | device();
}

std::string randomUuid() {
  static std::mutex mutex;
  static std::random_device device;
  std::uint64_t high = 0;
  std::uint64_t low = 0;
  {
    std::lock_guard lock(mutex);
    high = randomWord(device);
    low = randomWord(device);
  }
  high = (high & 0xffffffffffff0fffULL) | 0x0000000000004000ULL;
  low = (low & 0x3fffffffffffffffULL) | 0x8000000000000000ULL;
  return std::format(
      "{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
      high >> 32,
      (high >> 16) & 0xffff,
      high & 0xffff,
      low >> 48,
      low & 0xffffffffffffULL);
}

} // namespace

BlobStore &BlobStore::shared() {
  static BlobStore store;
  return store;
}

std::string BlobStore::store(std::string bytes) {
  auto blobId = randomUuid();
  store(blobId, std::move(bytes));
  return blobId;
}

void BlobStore::store(const std::string &blobId, std::string bytes) {
  auto shared = std::make_shared<const std::string>(std::move(bytes));
  std::lock_guard lock(mutex_);
  blobs_.insert_or_assign(blobId, std::move(shared));
}

std::optional<std::string> BlobStore::resolve(
    const std::string &blobId,
    std::size_t offset,
    std::optional<std::size_t> size) const {
  std::shared_ptr<const std::string> bytes;
  {
    std::lock_guard lock(mutex_);
    auto found = blobs_.find(blobId);
    if (found == blobs_.end()) return std::nullopt;
    bytes = found->second;
  }
  if (offset > bytes->size()) return std::nullopt;
  const auto length = size.value_or(bytes->size() - offset);
  if (length > bytes->size() - offset) return std::nullopt;
  return bytes->substr(offset, length);
}

std::size_t BlobStore::size(const std::string &blobId) const {
  std::lock_guard lock(mutex_);
  auto found = blobs_.find(blobId);
  return found == blobs_.end() ? 0 : found->second->size();
}

void BlobStore::remove(const std::string &blobId) {
  std::shared_ptr<const std::string> released;
  std::lock_guard lock(mutex_);
  if (auto found = blobs_.find(blobId); found != blobs_.end()) {
    released = std::move(found->second);
    blobs_.erase(found);
  }
}

} // namespace rntizen
