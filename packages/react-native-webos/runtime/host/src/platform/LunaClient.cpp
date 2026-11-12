#include "LunaClient.h"

#include "../logging/Log.h"
#include "../threading/MainThread.h"

#include <dlfcn.h>
#include <folly/json.h>

#include <bit>
#include <format>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace rnwebos {

namespace {

// lunaservice.h and GLib's gmain.h as the webOS toolchains ship them: both
// libraries are opened at run time, so the host links neither.
struct LSHandle;
struct LSMessage;
struct GMainContext;
struct GMainLoop;
using LSMessageToken = unsigned long;
using LSFilterFunc = bool (*)(LSHandle *handle, LSMessage *reply, void *context);
using GSourceFunc = int (*)(void *data);

struct LSError {
  int error_code;
  char *message;
  const char *file;
  int line;
  const char *func;
  void *padding;
  unsigned long magic;
};

using LSCallFunc = bool (*)(LSHandle *, const char *, const char *, LSFilterFunc, void *, LSMessageToken *, LSError *);

struct Api {
  bool (*errorInit)(LSError *){nullptr};
  void (*errorFree)(LSError *){nullptr};
  bool (*registerName)(const char *, LSHandle **, LSError *){nullptr};
  bool (*unregister)(LSHandle *, LSError *){nullptr};
  bool (*attach)(LSHandle *, GMainContext *, LSError *){nullptr};
  LSCallFunc call{nullptr};
  LSCallFunc callOneReply{nullptr};
  bool (*callCancel)(LSHandle *, LSMessageToken, LSError *){nullptr};
  const char *(*payload)(LSMessage *){nullptr};
  GMainContext *(*contextNew)(){nullptr};
  void (*contextUnref)(GMainContext *){nullptr};
  void (*contextInvoke)(GMainContext *, GSourceFunc, void *){nullptr};
  GMainLoop *(*loopNew)(GMainContext *, int){nullptr};
  void (*loopRun)(GMainLoop *){nullptr};
  void (*loopQuit)(GMainLoop *){nullptr};
  void (*loopUnref)(GMainLoop *){nullptr};
};

constexpr std::size_t kMaxReplyBytes = 4 * 1024 * 1024;

template <typename Function>
bool bind(void *library, const char *name, Function &function) {
  function = std::bit_cast<Function>(dlsym(library, name));
  return function != nullptr;
}

const Api *loadApi() {
  // Never closed: both libraries keep state for the life of the process.
  void *luna = dlopen("libluna-service2.so.3", RTLD_NOW | RTLD_GLOBAL);
  void *glib = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_GLOBAL);
  if (luna == nullptr || glib == nullptr) return nullptr;
  static Api api;
  const bool complete =
      bind(luna, "LSErrorInit", api.errorInit) && bind(luna, "LSErrorFree", api.errorFree) &&
      bind(luna, "LSRegister", api.registerName) && bind(luna, "LSUnregister", api.unregister) &&
      bind(luna, "LSGmainContextAttach", api.attach) && bind(luna, "LSCall", api.call) &&
      bind(luna, "LSCallOneReply", api.callOneReply) && bind(luna, "LSCallCancel", api.callCancel) &&
      bind(luna, "LSMessageGetPayload", api.payload) && bind(glib, "g_main_context_new", api.contextNew) &&
      bind(glib, "g_main_context_unref", api.contextUnref) && bind(glib, "g_main_context_invoke", api.contextInvoke) &&
      bind(glib, "g_main_loop_new", api.loopNew) && bind(glib, "g_main_loop_run", api.loopRun) &&
      bind(glib, "g_main_loop_quit", api.loopQuit) && bind(glib, "g_main_loop_unref", api.loopUnref);
  if (!complete) log(LogLevel::Warning, "libluna-service2.so.3 or GLib lacks a call the Luna client needs");
  return complete ? &api : nullptr;
}

const Api *lunaApi() {
  static const Api *const api = loadApi();
  return api;
}

std::string describe(const LSError &error) {
  return std::format("code {}: {}", error.error_code, error.message != nullptr ? error.message : "no message");
}

folly::dynamic parsed(const std::string &text) {
  try {
    return folly::parseJson(text);
  } catch (const folly::json::parse_error &) {
    return folly::dynamic::object("returnValue", false)("errorText", "The reply was not JSON");
  } catch (const std::range_error &) {
    return folly::dynamic::object("returnValue", false)("errorText", "The reply was not JSON");
  }
}

} // namespace

struct LunaClient::Registration {
  const Api *api{nullptr};
  LSHandle *handle{nullptr};
  GMainContext *context{nullptr};
  GMainLoop *loop{nullptr};
  std::string name;
  std::jthread thread;

  ~Registration() {
    // Queued rather than called, so it also ends a loop that has not started yet.
    api->contextInvoke(
        context,
        [](void *data) {
          auto *self = static_cast<Registration *>(data);
          self->api->loopQuit(self->loop);
          return 0;
        },
        this);
    if (thread.joinable()) thread.join();
    LSError error{};
    api->errorInit(&error);
    if (!api->unregister(handle, &error)) log(LogLevel::Debug, "Luna client " + name + " stays registered: " + describe(error));
    api->errorFree(&error);
    api->loopUnref(loop);
    api->contextUnref(context);
  }
};

namespace {

struct ClientCall {
  std::shared_ptr<LunaClient::Registration> registration;
  std::string uri;
  std::string payload;
  bool multiple{false};
  std::mutex mutex;
  LunaConnection::Reply onReply;
  LSMessageToken token{0};
};

// The registration joins its own thread when it goes, so its last reference
// is never dropped there.
void release(std::unique_ptr<ClientCall> call) {
  auto registration = std::move(call->registration);
  call.reset();
  runOnMainThread([registration = std::move(registration)]() mutable { registration.reset(); });
}

void deliver(ClientCall &call, std::string json) {
  LunaConnection::Reply handler;
  {
    std::lock_guard lock(call.mutex);
    handler = call.onReply;
  }
  if (!handler) return;
  if (json.size() > kMaxReplyBytes) json = "{}";
  runOnMainThread([handler = std::move(handler), json = std::move(json)] { handler(parsed(json)); });
}

// Runs on the client's own thread, like everything below that touches luna-service2.
bool onReply(LSHandle *, LSMessage *reply, void *context) {
  auto *call = static_cast<ClientCall *>(context);
  const char *text = call->registration->api->payload(reply);
  deliver(*call, text != nullptr ? text : "{}");
  if (!call->multiple) release(std::unique_ptr<ClientCall>(call));
  return true;
}

int sendCall(void *data) {
  auto *call = static_cast<ClientCall *>(data);
  const auto &registration = *call->registration;
  LSError error{};
  registration.api->errorInit(&error);
  const auto send = call->multiple ? registration.api->call : registration.api->callOneReply;
  const bool sent =
      send(registration.handle, call->uri.c_str(), call->payload.c_str(), onReply, call, &call->token, &error);
  if (!sent) {
    log(LogLevel::Warning, std::format("The Luna bus refused {}: {}", call->uri, describe(error)));
    deliver(*call, folly::toJson(folly::dynamic::object("returnValue", false)("errorText", describe(error))));
    call->token = 0;
  }
  registration.api->errorFree(&error);
  if (!sent && !call->multiple) release(std::unique_ptr<ClientCall>(call));
  return 0;
}

int cancelCall(void *data) {
  auto *call = static_cast<ClientCall *>(data);
  const auto &registration = *call->registration;
  if (call->token != 0) {
    LSError error{};
    registration.api->errorInit(&error);
    registration.api->callCancel(registration.handle, call->token, &error);
    registration.api->errorFree(&error);
  }
  release(std::unique_ptr<ClientCall>(call));
  return 0;
}

class ClientSubscription final : public LunaConnection::Subscription {
 public:
  explicit ClientSubscription(ClientCall *call) : call_(call) {}

  ~ClientSubscription() override {
    {
      std::lock_guard lock(call_->mutex);
      call_->onReply = nullptr;
    }
    const auto registration = call_->registration;
    registration->api->contextInvoke(registration->context, cancelCall, call_);
  }

  ClientSubscription(const ClientSubscription &) = delete;
  ClientSubscription &operator=(const ClientSubscription &) = delete;

 private:
  ClientCall *const call_;
};

ClientCall *queue(
    const std::shared_ptr<LunaClient::Registration> &registration,
    std::string uri,
    const folly::dynamic &payload,
    LunaConnection::Reply onReply,
    bool multiple) {
  auto call = std::make_unique<ClientCall>();
  call->registration = registration;
  call->uri = std::move(uri);
  call->payload = folly::toJson(payload);
  call->multiple = multiple;
  call->onReply = std::move(onReply);
  registration->api->contextInvoke(registration->context, sendCall, call.get());
  return call.release();
}

} // namespace

std::unique_ptr<LunaClient> LunaClient::registerAs(const std::string &name, std::string *refusal) {
  const auto refuse = [refusal](std::string why) {
    if (refusal != nullptr) *refusal = std::move(why);
    return nullptr;
  };
  const auto *api = lunaApi();
  if (api == nullptr) return nullptr;
  LSError error{};
  api->errorInit(&error);
  LSHandle *handle = nullptr;
  if (!api->registerName(name.c_str(), &handle, &error) || handle == nullptr) {
    auto why = describe(error);
    api->errorFree(&error);
    return refuse(std::move(why));
  }
  auto *context = api->contextNew();
  if (!api->attach(handle, context, &error)) {
    auto why = describe(error);
    api->errorFree(&error);
    api->errorInit(&error);
    api->unregister(handle, &error);
    api->errorFree(&error);
    api->contextUnref(context);
    return refuse(std::move(why));
  }
  api->errorFree(&error);
  auto registration = std::make_shared<Registration>();
  registration->api = api;
  registration->handle = handle;
  registration->context = context;
  registration->loop = api->loopNew(context, 0);
  registration->name = name;
  registration->thread = std::jthread([api, loop = registration->loop] { api->loopRun(loop); });
  return std::unique_ptr<LunaClient>(new LunaClient(std::move(registration)));
}

LunaClient::LunaClient(std::shared_ptr<Registration> registration) : registration_(std::move(registration)) {}

LunaClient::~LunaClient() = default;

bool LunaClient::isAvailable() const {
  return true;
}

bool LunaClient::call(const std::string &uri, const folly::dynamic &payload, Reply onReply) {
  queue(registration_, uri, payload, std::move(onReply), false);
  return true;
}

std::unique_ptr<LunaConnection::Subscription> LunaClient::subscribe(
    const std::string &uri, const folly::dynamic &payload, Reply onReply) {
  return std::make_unique<ClientSubscription>(queue(registration_, uri, payload, std::move(onReply), true));
}

} // namespace rnwebos
