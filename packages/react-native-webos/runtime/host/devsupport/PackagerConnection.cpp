#include "PackagerConnection.h"

#include "PackagerMessage.h"

#include "logging/Log.h"
#include "threading/MainThread.h"

namespace rntizen {

namespace {

constexpr double kRetrySeconds = 2.0;

} // namespace

PackagerConnection::PackagerConnection(std::string url, Handlers handlers)
    : url_(std::move(url)), handlers_(std::move(handlers)), lifetime_(std::make_shared<PackagerConnection *>(this)) {
  open();
}

PackagerConnection::~PackagerConnection() {
  lifetime_.reset();
  if (retryTimer_ != nullptr) ecore_timer_del(retryTimer_);
}

void PackagerConnection::open() {
  auto onMainThread = [weak = std::weak_ptr<PackagerConnection *>(lifetime_)](
                          std::function<void(PackagerConnection &)> task) {
    runOnMainThread([weak, task = std::move(task)] {
      if (auto self = weak.lock()) task(**self);
    });
  };
  socket_ = std::make_unique<WebSocketConnection>(
      WebSocketRequest{.url = url_, .protocols = {}, .headers = {}},
      WebSocketListener{
          .onOpen =
              [onMainThread](const std::string &) {
                onMainThread([](PackagerConnection &self) {
                  self.reportedFailure_ = false;
                  log(LogLevel::Info, "Connected to the dev server at " + self.url_);
                });
              },
          .onMessage =
              [onMainThread](std::string data, bool binary) {
                if (binary) return;
                onMainThread([data = std::move(data)](PackagerConnection &self) { self.receive(data); });
              },
          .onClosed =
              [onMainThread](int, const std::string &) {
                onMainThread([](PackagerConnection &self) { self.retryLater(); });
              },
          .onFailed =
              [onMainThread](const std::string &message) {
                onMainThread([message](PackagerConnection &self) {
                  if (!self.reportedFailure_) log(LogLevel::Warning, "No dev server at " + self.url_ + ": " + message);
                  self.reportedFailure_ = true;
                  self.retryLater();
                });
              },
      });
}

void PackagerConnection::retryLater() {
  if (retryTimer_ != nullptr) return;
  retryTimer_ = ecore_timer_add(kRetrySeconds, retry, this);
}

Eina_Bool PackagerConnection::retry(void *data) {
  auto &self = *static_cast<PackagerConnection *>(data);
  self.retryTimer_ = nullptr;
  self.socket_.reset();
  self.open();
  return ECORE_CALLBACK_CANCEL;
}

void PackagerConnection::receive(std::string_view text) {
  const auto message = readPackagerMessage(text);
  if (message.command == PackagerCommand::Reload && handlers_.reload) handlers_.reload();
  if (message.command == PackagerCommand::DevMenu && handlers_.devMenu) handlers_.devMenu();
  if (message.reply.has_value() && socket_ != nullptr) socket_->send(WebSocketOpcode::Text, *message.reply);
}

} // namespace rntizen
