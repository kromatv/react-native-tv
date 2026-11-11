#include "LunaBus.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <dlfcn.h>
#include <folly/json.h>

#include <bit>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace rnwebos {

namespace {

// libhelpers.h as webosbrew publishes it: LG ships no header.
struct LSHandle;
struct LSMessage;
struct HContext;
using LSFilterFunc = bool (*)(LSHandle *handle, LSMessage *reply, void *context);

struct HContext {
  LSFilterFunc callback;
  void *userdata;
  void *unknown;
  int multiple;
  int pub;
  unsigned long ret_token;
};

using HLunaServiceCall = int (*)(const char *uri, const char *payload, HContext *context);
using HUnregisterServiceCallback = int (*)(HContext *context);
using HLunaServiceMessage = const char *(*)(LSMessage *message);

constexpr std::size_t kMaxReplyBytes = 4 * 1024 * 1024;

struct Call {
  HContext context{};
  HLunaServiceMessage message{nullptr};
  std::mutex mutex;
  LunaBus::Reply onReply;
  bool multiple{false};
  bool ended{false};
};

folly::dynamic parsed(const std::string &text) {
  try {
    return folly::parseJson(text);
  } catch (const std::exception &) {
    return folly::dynamic::object("returnValue", false)("errorText", "The reply was not JSON");
  }
}

// Runs on libhelpers' own thread.
bool onReply(LSHandle *, LSMessage *reply, void *context) {
  auto *call = static_cast<Call *>(static_cast<HContext *>(context)->userdata);
  const char *text = call->message(reply);
  std::string json = text != nullptr ? text : "{}";
  if (json.size() > kMaxReplyBytes) json = "{}";
  LunaBus::Reply handler;
  {
    std::lock_guard lock(call->mutex);
    if (!call->ended) handler = call->onReply;
  }
  if (handler) {
    runOnMainThread([handler = std::move(handler), json = std::move(json)] { handler(parsed(json)); });
  }
  if (!call->multiple) delete call;
  return true;
}

class LunaSubscription final : public LunaBus::Subscription {
 public:
  LunaSubscription(Call *call, HUnregisterServiceCallback unregister) : call_(call), unregister_(unregister) {}

  // The call is never freed: libhelpers may be delivering a reply to it on
  // its own thread while this runs, and gives no way to wait for that.
  ~LunaSubscription() override {
    {
      std::lock_guard lock(call_->mutex);
      call_->ended = true;
      call_->onReply = nullptr;
    }
    unregister_(&call_->context);
  }

 private:
  Call *const call_;
  const HUnregisterServiceCallback unregister_;
};

} // namespace

struct LunaBus::Library {
  HLunaServiceCall call{nullptr};
  HUnregisterServiceCallback unregister{nullptr};
  HLunaServiceMessage message{nullptr};

  Call *send(const std::string &uri, const folly::dynamic &payload, Reply reply, bool multiple) const {
    if (call == nullptr) return nullptr;
    auto *sent = new Call();
    sent->message = message;
    sent->onReply = std::move(reply);
    sent->multiple = multiple;
    sent->context.callback = onReply;
    sent->context.userdata = sent;
    sent->context.multiple = multiple ? 1 : 0;
    sent->context.pub = 1;
    if (call(uri.c_str(), folly::toJson(payload).c_str(), &sent->context) == 0) return sent;
    log(LogLevel::Warning, "The Luna bus refused " + uri);
    delete sent;
    return nullptr;
  }
};

LunaBus::LunaBus() : library_(std::make_unique<Library>()) {
  // Never closed: libhelpers starts a thread that has no way to stop.
  void *helpers = dlopen("libhelpers.so.2", RTLD_NOW | RTLD_GLOBAL);
  if (helpers == nullptr) return;
  const auto call = std::bit_cast<HLunaServiceCall>(dlsym(helpers, "HLunaServiceCall"));
  const auto unregister = std::bit_cast<HUnregisterServiceCallback>(dlsym(helpers, "HUnregisterServiceCallback"));
  const auto message = std::bit_cast<HLunaServiceMessage>(dlsym(helpers, "HLunaServiceMessage"));
  if (call == nullptr || unregister == nullptr || message == nullptr) {
    log(LogLevel::Warning, "libhelpers.so.2 lacks the Luna calls: the bus is unavailable");
    return;
  }
  library_->call = call;
  library_->unregister = unregister;
  library_->message = message;
}

LunaBus &LunaBus::get() {
  static LunaBus *bus = new LunaBus();
  return *bus;
}

bool LunaBus::isAvailable() const {
  return library_->call != nullptr;
}

bool LunaBus::call(const std::string &uri, const folly::dynamic &payload, Reply onReply) {
  return library_->send(uri, payload, std::move(onReply), false) != nullptr;
}

std::optional<folly::dynamic> LunaBus::callAndWait(
    const std::string &uri,
    const folly::dynamic &payload,
    std::chrono::milliseconds timeout) {
  struct Wait {
    std::mutex mutex;
    std::condition_variable answered;
    std::optional<folly::dynamic> reply;
  };
  if (!isAvailable()) return std::nullopt;
  auto wait = std::make_shared<Wait>();
  // The reply handler normally hops to the main thread, which may be the one
  // waiting here: this one is taken on libhelpers' thread instead.
  auto *sent = new Call();
  sent->message = library_->message;
  sent->multiple = false;
  sent->context.userdata = sent;
  sent->context.multiple = 0;
  sent->context.pub = 1;
  sent->onReply = [wait](const folly::dynamic &reply) {
    std::lock_guard lock(wait->mutex);
    wait->reply = reply;
    wait->answered.notify_all();
  };
  sent->context.callback = [](LSHandle *, LSMessage *reply, void *context) {
    auto *call = static_cast<Call *>(static_cast<HContext *>(context)->userdata);
    const char *text = call->message(reply);
    const std::string json = text != nullptr ? text : "{}";
    call->onReply(json.size() > kMaxReplyBytes ? folly::dynamic::object() : parsed(json));
    delete call;
    return true;
  };
  if (library_->call(uri.c_str(), folly::toJson(payload).c_str(), &sent->context) != 0) {
    delete sent;
    return std::nullopt;
  }
  std::unique_lock lock(wait->mutex);
  wait->answered.wait_for(lock, timeout, [&wait] { return wait->reply.has_value(); });
  return wait->reply;
}

std::unique_ptr<LunaBus::Subscription> LunaBus::subscribe(
    const std::string &uri,
    const folly::dynamic &payload,
    Reply onReply) {
  auto *call = library_->send(uri, payload, std::move(onReply), true);
  if (call == nullptr) return nullptr;
  return std::make_unique<LunaSubscription>(call, library_->unregister);
}

} // namespace rnwebos
