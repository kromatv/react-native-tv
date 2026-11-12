#pragma once

#include "LunaBus.h"

#include "../platform/MainLoop.h"

#include <folly/json.h>

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rnwebos::testing {

/// The Luna services of a TV that is not there: a test says what each URI
/// answers, as literal JSON, and reads back what the host asked.
class FakeLuna final {
 public:
  struct Call {
    std::string uri;
    folly::dynamic payload;
  };

  /// What a one-shot call or a new subscription to `uri` is answered with.
  void answer(const std::string &uri, std::string json) {
    std::lock_guard lock(state_->mutex);
    state_->answers[uri] = std::move(json);
  }

  /// Makes the bus refuse every call to `uri`.
  void refuse(const std::string &uri) {
    std::lock_guard lock(state_->mutex);
    state_->refused.push_back(uri);
  }

  void setAvailable(bool available) {
    std::lock_guard lock(state_->mutex);
    state_->available = available;
  }

  /// Sends one more reply to every live subscription to `uri`, then runs the main loop once.
  void publish(const std::string &uri, const std::string &json) {
    std::vector<LunaTransport::OnMessage> handlers;
    {
      std::lock_guard lock(state_->mutex);
      for (const auto &[id, subscription] : state_->subscriptions) {
        if (subscription.uri == uri) handlers.push_back(subscription.onMessage);
      }
    }
    for (const auto &handler : handlers) handler(json);
    deliver();
  }

  /// Runs the replies the bus queued for the main thread.
  static void deliver() {
    MainLoop::get().iterate();
  }

  std::vector<Call> calls() const {
    std::lock_guard lock(state_->mutex);
    return state_->calls;
  }

  /// The payloads sent to `uri`, in order.
  std::vector<folly::dynamic> callsTo(const std::string &uri) const {
    std::vector<folly::dynamic> payloads;
    for (const auto &call : calls()) {
      if (call.uri == uri) payloads.push_back(call.payload);
    }
    return payloads;
  }

  bool isSubscribed(const std::string &uri) const {
    std::lock_guard lock(state_->mutex);
    return std::ranges::any_of(state_->subscriptions, [&uri](const auto &entry) { return entry.second.uri == uri; });
  }

  std::unique_ptr<LunaTransport> transport() const {
    return std::make_unique<Transport>(state_);
  }

 private:
  struct Subscribed {
    std::string uri;
    LunaTransport::OnMessage onMessage;
  };

  struct State {
    std::mutex mutex;
    bool available{true};
    std::map<std::string, std::string, std::less<>> answers;
    std::vector<std::string> refused;
    std::vector<Call> calls;
    std::map<int, Subscribed> subscriptions;
    int nextSubscription{1};

    bool record(const std::string &uri, const std::string &payload) {
      if (!available || std::ranges::find(refused, uri) != refused.end()) return false;
      calls.push_back({uri, folly::parseJson(payload)});
      return true;
    }
  };

  class Unsubscriber final : public LunaTransport::Subscription {
   public:
    Unsubscriber(std::shared_ptr<State> state, int id) : state_(std::move(state)), id_(id) {}
    ~Unsubscriber() override {
      std::lock_guard lock(state_->mutex);
      state_->subscriptions.erase(id_);
    }

   private:
    const std::shared_ptr<State> state_;
    const int id_;
  };

  class Transport final : public LunaTransport {
   public:
    explicit Transport(std::shared_ptr<State> state) : state_(std::move(state)) {}

    bool isAvailable() const override {
      std::lock_guard lock(state_->mutex);
      return state_->available;
    }

    bool send(const std::string &uri, const std::string &payload, OnMessage onMessage) override {
      std::string answer;
      {
        std::lock_guard lock(state_->mutex);
        if (!state_->record(uri, payload)) return false;
        const auto found = state_->answers.find(uri);
        if (found == state_->answers.end()) return true;
        answer = found->second;
      }
      onMessage(answer);
      return true;
    }

    std::unique_ptr<LunaTransport::Subscription> subscribe(
        const std::string &uri,
        const std::string &payload,
        OnMessage onMessage) override {
      std::string answer;
      int id = 0;
      {
        std::lock_guard lock(state_->mutex);
        if (!state_->record(uri, payload)) return nullptr;
        id = state_->nextSubscription++;
        state_->subscriptions[id] = {uri, onMessage};
        const auto found = state_->answers.find(uri);
        if (found != state_->answers.end()) answer = found->second;
      }
      if (!answer.empty()) onMessage(answer);
      return std::make_unique<Unsubscriber>(state_, id);
    }

   private:
    const std::shared_ptr<State> state_;
  };

  const std::shared_ptr<State> state_{std::make_shared<State>()};
};

} // namespace rnwebos::testing
