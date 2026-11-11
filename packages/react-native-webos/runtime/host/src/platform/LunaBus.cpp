#include "LunaBus.h"

#include "../threading/MainThread.h"

#include <folly/Indestructible.h>
#include <folly/json.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace rnwebos {

namespace {

constexpr std::size_t kMaxReplyBytes = 4 * 1024 * 1024;

folly::dynamic failure(const char *why) {
  return folly::dynamic::object("returnValue", false)("errorText", why);
}

folly::dynamic parsed(const std::string &text) {
  if (text.size() > kMaxReplyBytes) return failure("The reply was too large");
  try {
    return folly::parseJson(text);
  } catch (const folly::json::parse_error &) {
    return failure("The reply was not JSON");
  } catch (const std::range_error &) {
    return failure("The reply was not JSON");
  }
}

class WaitedReply final : public LunaBus::PendingReply {
 public:
  void set(folly::dynamic reply) {
    std::lock_guard lock(mutex_);
    reply_ = std::move(reply);
    answered_.notify_all();
  }

  std::optional<folly::dynamic> waitUntil(std::chrono::steady_clock::time_point deadline) override {
    std::unique_lock lock(mutex_);
    answered_.wait_until(lock, deadline, [this] { return reply_.has_value(); });
    return reply_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable answered_;
  std::optional<folly::dynamic> reply_;
};

class BusSubscription final : public LunaBus::Subscription {
 public:
  explicit BusSubscription(std::shared_ptr<std::atomic<bool>> ended) : ended_(std::move(ended)) {}

  ~BusSubscription() override {
    ended_->store(true);
    call_.reset();
  }

  void hold(std::unique_ptr<LunaTransport::Subscription> call) {
    call_ = std::move(call);
  }

 private:
  const std::shared_ptr<std::atomic<bool>> ended_;
  std::unique_ptr<LunaTransport::Subscription> call_;
};

} // namespace

LunaBus::LunaBus(std::unique_ptr<LunaTransport> transport) : transport_(std::move(transport)) {}

LunaBus::~LunaBus() = default;

LunaBus &LunaBus::get() {
  static folly::Indestructible<LunaBus> bus(makeLibHelpersTransport());
  return *bus;
}

bool LunaBus::isAvailable() const {
  return transport_->isAvailable();
}

bool LunaBus::call(const std::string &uri, const folly::dynamic &payload, Reply onReply) {
  if (!isAvailable()) return false;
  return transport_->send(uri, folly::toJson(payload), [onReply = std::move(onReply)](std::string json) {
    if (!onReply) return;
    runOnMainThread([onReply, json = std::move(json)] { onReply(parsed(json)); });
  });
}

std::shared_ptr<LunaBus::PendingReply> LunaBus::ask(const std::string &uri, const folly::dynamic &payload) {
  if (!isAvailable()) return nullptr;
  auto reply = std::make_shared<WaitedReply>();
  const bool sent =
      transport_->send(uri, folly::toJson(payload), [reply](const std::string &json) { reply->set(parsed(json)); });
  return sent ? reply : nullptr;
}

std::optional<folly::dynamic> LunaBus::callAndWait(
    const std::string &uri,
    const folly::dynamic &payload,
    std::chrono::milliseconds timeout) {
  const auto reply = ask(uri, payload);
  if (reply == nullptr) return std::nullopt;
  return reply->waitUntil(std::chrono::steady_clock::now() + timeout);
}

std::unique_ptr<LunaBus::Subscription> LunaBus::subscribe(
    const std::string &uri,
    const folly::dynamic &payload,
    Reply onReply) {
  if (!isAvailable()) return nullptr;
  auto ended = std::make_shared<std::atomic<bool>>(false);
  auto subscription = std::make_unique<BusSubscription>(ended);
  auto call = transport_->subscribe(
      uri, folly::toJson(payload), [ended, onReply = std::move(onReply)](std::string json) {
        runOnMainThread([ended, onReply, json = std::move(json)] {
          if (!ended->load()) onReply(parsed(json));
        });
      });
  if (call == nullptr) return nullptr;
  subscription->hold(std::move(call));
  return subscription;
}

} // namespace rnwebos
