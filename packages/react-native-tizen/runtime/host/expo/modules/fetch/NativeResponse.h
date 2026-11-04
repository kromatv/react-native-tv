#pragma once

#include "../../ModuleDefinition.h"

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rntizen::expo {

/// Bytes JavaScript receives as an ArrayBuffer rather than a Uint8Array.
struct ArrayBufferBytes {
  std::string data;
};

template <>
struct Convert<ArrayBufferBytes> {
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const ArrayBufferBytes &bytes) {
    return Convert<Bytes>::toJs(runtime, Bytes{bytes.data}).getObject(runtime).getProperty(runtime, "buffer");
  }
};

/// The native side of `expo/fetch`'s response: filled from the HTTP engine's
/// thread, read by JavaScript. The body arrives whole, so a stream gets it as
/// one chunk. Thread-safe.
class NativeResponse final : public SharedObject {
 public:
  explicit NativeResponse(std::shared_ptr<Context> context);

  /// Holds `promise` until the response head arrives; rejects it at once, and
  /// returns false, when the response already ended, as a cancelled one has.
  bool awaitHead(Promise promise);
  void receiveHead(long status, std::vector<std::pair<std::string, std::string>> headers, std::string url, bool redirected);
  void receiveBody(std::string_view body);
  void complete(const std::string &error);

  folly::dynamic rawHeaders() const;
  long status() const;
  std::string url() const;
  bool redirected() const;
  bool bodyUsed() const;

  void startStreaming(const Promise &promise);
  void stopStreaming();
  void readBody(Promise promise, bool asText);

 private:
  struct Reader {
    Promise promise;
    bool asText;
  };

  void settle(const Reader &reader) const;
  void emit(const std::string &event, JsValueFactory payload) const;

  const std::shared_ptr<Context> context_;
  mutable std::mutex mutex_;
  std::optional<Promise> head_;
  long status_{0};
  std::vector<std::pair<std::string, std::string>> headers_;
  std::string url_;
  bool redirected_{false};
  std::string body_;
  bool completed_{false};
  std::optional<std::string> error_;
  bool streaming_{false};
  bool bodyUsed_{false};
  std::vector<Reader> readers_;
};

ClassDefinition<NativeResponse> nativeResponseClass(std::shared_ptr<Context> context);

} // namespace rntizen::expo
