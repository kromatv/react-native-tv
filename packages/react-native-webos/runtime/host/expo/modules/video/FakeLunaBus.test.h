#pragma once

#include "platform/LunaConnection.h"

#include <folly/json.h>

#include <memory>
#include <string>
#include <vector>

namespace rnwebos::expo::testing {

/// A Luna bus a test plays the service on: it keeps every call, answers the
/// ones the test picks with the JSON the test gives, and sends events to the
/// subscriptions still open. An `unload` is followed by its `unloadCompleted`
/// at once, unless the test wants to send that itself.
class FakeLunaBus final : public LunaConnection {
 public:
  struct Call {
    std::string method;
    folly::dynamic payload;
    Reply onReply;
    std::shared_ptr<bool> open;
  };

  bool isAvailable() const override {
    return true;
  }

  bool call(const std::string &uri, const folly::dynamic &payload, Reply onReply = {}) override {
    if (refuseCalls) return false;
    calls.push_back({.method = methodOf(uri), .payload = payload, .onReply = std::move(onReply), .open = nullptr});
    if (completesUnloads && calls.back().method == "unload") emit(R"({"unloadCompleted":{}})");
    return true;
  }

  std::unique_ptr<Subscription> subscribe(const std::string &uri, const folly::dynamic &payload, Reply onReply) override {
    auto open = std::make_shared<bool>(true);
    calls.push_back({.method = methodOf(uri), .payload = payload, .onReply = std::move(onReply), .open = open});
    return std::make_unique<Open>(std::move(open));
  }

  /// The methods called so far, in order, joined with spaces.
  std::string methods() const {
    std::string joined;
    for (const auto &call : calls) joined += (joined.empty() ? "" : " ") + call.method;
    return joined;
  }

  int count(const std::string &method) const {
    int total = 0;
    for (const auto &call : calls) total += call.method == method ? 1 : 0;
    return total;
  }

  /// The payload of the last call of `method` as JSON with sorted keys, empty when there was none.
  std::string payloadOf(const std::string &method) const {
    const auto *call = last(method);
    if (call == nullptr) return {};
    folly::json::serialization_opts options;
    options.sort_keys = true;
    return folly::json::serialize(call->payload, options);
  }

  /// Answers the last call of `method` with `json`.
  void answer(const std::string &method, const std::string &json) const {
    const auto *call = last(method);
    if (call != nullptr && call->onReply) call->onReply(folly::parseJson(json));
  }

  /// Sends `json` to every subscription still open.
  void emit(const std::string &json) const {
    const auto calls = this->calls;
    for (const auto &call : calls) {
      if (call.open && *call.open && call.onReply) call.onReply(folly::parseJson(json));
    }
  }

  int openSubscriptions() const {
    int total = 0;
    for (const auto &call : calls) total += call.open && *call.open ? 1 : 0;
    return total;
  }

  std::vector<Call> calls;
  bool refuseCalls{false};
  bool completesUnloads{true};

 private:
  class Open final : public Subscription {
   public:
    explicit Open(std::shared_ptr<bool> open) : open_(std::move(open)) {}
    ~Open() override {
      *open_ = false;
    }

   private:
    const std::shared_ptr<bool> open_;
  };

  static std::string methodOf(const std::string &uri) {
    return uri.substr(uri.rfind('/') + 1);
  }

  const Call *last(const std::string &method) const {
    for (auto call = calls.rbegin(); call != calls.rend(); ++call) {
      if (call->method == method) return &*call;
    }
    return nullptr;
  }
};

} // namespace rnwebos::expo::testing
