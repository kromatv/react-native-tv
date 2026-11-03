#include "BlobReference.h"

#include "BlobStore.h"

namespace rntizen {

namespace {

std::optional<double> number(facebook::jsi::Runtime &runtime, const facebook::jsi::Object &data, const char *name) {
  auto value = data.getProperty(runtime, name);
  if (!value.isNumber()) return std::nullopt;
  return value.getNumber();
}

} // namespace

BlobReference readBlobReference(facebook::jsi::Runtime &runtime, const facebook::jsi::Object &data) {
  auto blobId = data.getProperty(runtime, "blobId");
  if (!blobId.isString()) throw facebook::jsi::JSError(runtime, "The specified blob does not contain a blobId");
  auto type = data.getProperty(runtime, "type");
  const auto size = number(runtime, data, "size");
  return BlobReference{
      .blobId = blobId.getString(runtime).utf8(runtime),
      .offset = static_cast<std::size_t>(std::max(0.0, number(runtime, data, "offset").value_or(0))),
      .size = size && *size >= 0 ? std::optional(static_cast<std::size_t>(*size)) : std::nullopt,
      .type = type.isString() ? type.getString(runtime).utf8(runtime) : std::string(),
  };
}

std::optional<std::string> resolveBlob(const BlobReference &reference) {
  return BlobStore::shared().resolve(reference.blobId, reference.offset, reference.size);
}

} // namespace rntizen
