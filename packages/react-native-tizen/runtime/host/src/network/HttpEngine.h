#pragma once

#include <folly/dynamic.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rntizen {

/// One HTTP request as JavaScript's XMLHttpRequest describes it.
struct HttpRequest {
  int id;
  std::string method;
  std::string url;
  std::vector<std::pair<std::string, std::string>> headers;
  std::optional<std::string> body;
  std::chrono::milliseconds timeout;
};

/// What a finished request reports. Each callback runs once, in this order, on
/// the engine's thread; `onComplete` alone runs for a request that failed.
struct HttpResponseHandler {
  std::function<void(long status, folly::dynamic headers, std::string url)> onResponse;
  std::function<void(std::string body)> onBody;
  std::function<void(std::string error, bool timedOut)> onComplete;
};

/// Runs HTTP requests concurrently on one thread with libcurl's multi
/// interface, sharing cookies between them. Thread-safe.
class HttpEngine final {
 public:
  HttpEngine();
  ~HttpEngine();

  HttpEngine(const HttpEngine &) = delete;
  HttpEngine &operator=(const HttpEngine &) = delete;

  void start(HttpRequest request, HttpResponseHandler handler);
  void cancel(int requestId);
  void clearCookies(std::function<void()> done);

 private:
  struct Transfer;

  void post(std::function<void()> task);
  void run();
  void begin(HttpRequest request, HttpResponseHandler handler);
  void finish(void *easy, int result);
  void drop(int requestId);

  std::mutex mutex_;
  std::deque<std::function<void()>> tasks_;
  bool stopping_{false};
  void *multi_;
  void *share_;
  std::map<int, std::unique_ptr<Transfer>> transfers_;
  std::thread thread_;
};

} // namespace rntizen
