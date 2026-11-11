#include "LunaTransport.h"

#include "../logging/Log.h"

#include <dlfcn.h>

#include <bit>
#include <mutex>
#include <utility>

namespace rnwebos {

namespace {

// libhelpers.h as webosbrew publishes it: LG ships no header.
struct LSHandle;
struct LSMessage;
struct HContext;
using HLSFilterFunc = int (*)(LSHandle *handle, LSMessage *reply, HContext *context);

struct HContext {
  HLSFilterFunc callback;
  void *userdata;
  void *unknown;
  int multiple;
  int pub;
  unsigned long ret_token;
};

using HLunaServiceCall = int (*)(const char *uri, const char *payload, HContext *context);
using HUnregisterServiceCallback = int (*)(HContext *context);
using HLunaServiceMessage = const char *(*)(LSMessage *message);
using HLunaServiceBridgeInstance = void *(*)();

struct Call {
  HContext context{};
  HLunaServiceMessage message{nullptr};
  std::mutex mutex;
  LunaTransport::OnMessage onMessage;
  bool multiple{false};
};

// Runs on libhelpers' own thread.
int onReply(LSHandle *, LSMessage *reply, HContext *context) {
  auto *call = static_cast<Call *>(context->userdata);
  const std::unique_ptr<Call> answeredOnce(call->multiple ? nullptr : call);
  const char *text = call->message(reply);
  LunaTransport::OnMessage handler;
  {
    std::lock_guard lock(call->mutex);
    handler = call->onMessage;
  }
  if (handler) handler(text != nullptr ? text : "{}");
  return 1;
}

class LibHelpersSubscription final : public LunaTransport::Subscription {
 public:
  LibHelpersSubscription(Call *call, HUnregisterServiceCallback unregister) : call_(call), unregister_(unregister) {}

  // The call is never freed: libhelpers may be delivering a reply to it on
  // its own thread while this runs, and gives no way to wait for that.
  ~LibHelpersSubscription() override {
    {
      std::lock_guard lock(call_->mutex);
      call_->onMessage = nullptr;
    }
    unregister_(&call_->context);
  }

  LibHelpersSubscription(const LibHelpersSubscription &) = delete;
  LibHelpersSubscription &operator=(const LibHelpersSubscription &) = delete;

 private:
  Call *const call_;
  const HUnregisterServiceCallback unregister_;
};

class LibHelpersTransport final : public LunaTransport {
 public:
  LibHelpersTransport() {
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
    // HLunaServiceCall never returns when libhelpers cannot register on the
    // bus, as for a binary started from a shell: making its bridge first tells
    // (docs/reference/webos-tv-platform.md, section 4.1).
    if (const auto bridge =
            std::bit_cast<HLunaServiceBridgeInstance>(dlsym(helpers, "_ZN18HLunaServiceBridge8instanceEv"));
        bridge != nullptr && bridge() == nullptr) {
      log(LogLevel::Warning, "libhelpers cannot register on the Luna bus: the bus is unavailable");
      return;
    }
    call_ = call;
    unregister_ = unregister;
    message_ = message;
  }

  bool isAvailable() const override {
    return call_ != nullptr;
  }

  bool send(const std::string &uri, const std::string &payload, OnMessage onMessage) override {
    return start(uri, payload, std::move(onMessage), false) != nullptr;
  }

  std::unique_ptr<Subscription> subscribe(const std::string &uri, const std::string &payload, OnMessage onMessage) override {
    auto *call = start(uri, payload, std::move(onMessage), true);
    if (call == nullptr) return nullptr;
    return std::make_unique<LibHelpersSubscription>(call, unregister_);
  }

 private:
  Call *start(const std::string &uri, const std::string &payload, OnMessage onMessage, bool multiple) const {
    if (call_ == nullptr) return nullptr;
    auto sent = std::make_unique<Call>();
    sent->message = message_;
    sent->onMessage = std::move(onMessage);
    sent->multiple = multiple;
    sent->context.callback = onReply;
    sent->context.userdata = sent.get();
    sent->context.multiple = multiple ? 1 : 0;
    sent->context.pub = 1;
    if (call_(uri.c_str(), payload.c_str(), &sent->context) == 0) return sent.release();
    log(LogLevel::Warning, "The Luna bus refused " + uri);
    return nullptr;
  }

  HLunaServiceCall call_{nullptr};
  HUnregisterServiceCallback unregister_{nullptr};
  HLunaServiceMessage message_{nullptr};
};

} // namespace

std::unique_ptr<LunaTransport> makeLibHelpersTransport() {
  return std::make_unique<LibHelpersTransport>();
}

} // namespace rnwebos
