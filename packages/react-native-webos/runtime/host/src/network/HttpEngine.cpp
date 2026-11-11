#include "HttpEngine.h"

#include "CurlMethod.h"
#include "HeaderField.h"

#include <curl/curl.h>
#include <pthread.h>

#include <format>

namespace rntizen {

namespace {

constexpr int kPollTimeoutMs = 1000;
// Android and iOS send a User-Agent on every request, and some APIs refuse one
// without it. A User-Agent header from JavaScript still replaces this one.
constexpr const char *kDefaultUserAgent = "react-native-tizen";

std::string trimmed(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) return {};
  const auto last = text.find_last_not_of(" \t\r\n");
  return std::string(text.substr(first, last - first + 1));
}

} // namespace

struct HttpEngine::Transfer {
  HttpRequest request;
  HttpResponseHandler handler;
  CURL *easy{curl_easy_init()};
  curl_slist *headerList{nullptr};
  std::string body;
  folly::dynamic headers = folly::dynamic::object();

  Transfer(HttpRequest sent, HttpResponseHandler handling)
      : request(std::move(sent)), handler(std::move(handling)) {}

  Transfer(const Transfer &) = delete;
  Transfer &operator=(const Transfer &) = delete;

  ~Transfer() {
    if (headerList != nullptr) curl_slist_free_all(headerList);
    if (easy != nullptr) curl_easy_cleanup(easy);
  }

  static std::size_t appendBody(char *data, std::size_t size, std::size_t count, void *self) {
    static_cast<Transfer *>(self)->body.append(data, size * count);
    return size * count;
  }

  static std::size_t appendHeader(char *data, std::size_t size, std::size_t count, void *self) {
    auto &transfer = *static_cast<Transfer *>(self);
    const std::string_view line(data, size * count);
    if (line.starts_with("HTTP/")) {
      transfer.headers = folly::dynamic::object();
      return size * count;
    }
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) return size * count;
    const auto name = trimmed(line.substr(0, colon));
    const auto value = trimmed(line.substr(colon + 1));
    if (auto *existing = transfer.headers.get_ptr(name)) {
      *existing = std::format("{}, {}", existing->asString(), value);
    } else {
      transfer.headers[name] = value;
    }
    return size * count;
  }

  void setMethodAndBody() const {
    const auto method = curlMethodFor(request.method, request.body);
    if (method.noBody) curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
    if (method.customRequest) curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.customRequest->c_str());
    if (method.body) {
      curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(method.body->size()));
      curl_easy_setopt(easy, CURLOPT_POSTFIELDS, method.body->data());
    }
  }
};

HttpEngine::HttpEngine() : multi_(curl_multi_init()), share_(curl_share_init()) {
  curl_share_setopt(static_cast<CURLSH *>(share_), CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  thread_ = std::jthread([this] { run(); });
}

HttpEngine::~HttpEngine() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  curl_multi_wakeup(static_cast<CURLM *>(multi_));
  thread_.join();
  dropAll();
  curl_multi_cleanup(static_cast<CURLM *>(multi_));
  curl_share_cleanup(static_cast<CURLSH *>(share_));
}

void HttpEngine::post(std::function<void()> task) {
  {
    std::lock_guard lock(mutex_);
    tasks_.push_back(std::move(task));
  }
  curl_multi_wakeup(static_cast<CURLM *>(multi_));
}

void HttpEngine::start(HttpRequest request, HttpResponseHandler handler) {
  post([this, request = std::move(request), handler = std::move(handler)]() mutable {
    begin(std::move(request), std::move(handler));
  });
}

void HttpEngine::cancel(int requestId) {
  post([this, requestId] { drop(requestId); });
}

void HttpEngine::clearCookies(std::function<void()> done) {
  post([this, done = std::move(done)] {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy(curl_easy_init(), &curl_easy_cleanup);
    curl_easy_setopt(easy.get(), CURLOPT_SHARE, share_);
    curl_easy_setopt(easy.get(), CURLOPT_COOKIELIST, "ALL");
    done();
  });
}

void HttpEngine::begin(HttpRequest request, HttpResponseHandler handler) {
  if (const auto header = invalidHeader(request.headers)) {
    handler.onComplete(std::format("The header {} holds a character HTTP forbids", *header), false);
    return;
  }
  auto transfer = std::make_unique<Transfer>(std::move(request), std::move(handler));
  auto *easy = transfer->easy;
  if (easy == nullptr) {
    transfer->handler.onComplete("libcurl could not start the request", false);
    return;
  }
  curl_easy_setopt(easy, CURLOPT_URL, transfer->request.url.c_str());
  curl_easy_setopt(easy, CURLOPT_PRIVATE, transfer.get());
  curl_easy_setopt(easy, CURLOPT_SHARE, share_);
  curl_easy_setopt(easy, CURLOPT_COOKIEFILE, "");
  curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(easy, CURLOPT_USERAGENT, kDefaultUserAgent);
  curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(easy, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(transfer->request.timeout.count()));
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, Transfer::appendBody);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, transfer.get());
  curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, Transfer::appendHeader);
  curl_easy_setopt(easy, CURLOPT_HEADERDATA, transfer.get());
  transfer->setMethodAndBody();
  for (const auto &[name, value] : transfer->request.headers) {
    transfer->headerList = curl_slist_append(transfer->headerList, std::format("{}: {}", name, value).c_str());
  }
  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, transfer->headerList);
  curl_multi_add_handle(static_cast<CURLM *>(multi_), easy);
  transfers_[transfer->request.id] = std::move(transfer);
}

void HttpEngine::drop(int requestId) {
  auto found = transfers_.find(requestId);
  if (found == transfers_.end()) return;
  curl_multi_remove_handle(static_cast<CURLM *>(multi_), found->second->easy);
  transfers_.erase(found);
}

void HttpEngine::dropAll() {
  for (const auto &[requestId, transfer] : transfers_) {
    curl_multi_remove_handle(static_cast<CURLM *>(multi_), transfer->easy);
  }
  transfers_.clear();
}

void HttpEngine::finish(void *easy, int result) {
  Transfer *transfer = nullptr;
  curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_PRIVATE, &transfer);
  if (transfer == nullptr) return;
  if (const auto code = static_cast<CURLcode>(result); code != CURLE_OK) {
    transfer->handler.onComplete(curl_easy_strerror(code), code == CURLE_OPERATION_TIMEDOUT);
  } else {
    long status = 0;
    char *effectiveUrl = nullptr;
    curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_EFFECTIVE_URL, &effectiveUrl);
    transfer->handler.onResponse(status, transfer->headers, effectiveUrl != nullptr ? effectiveUrl : transfer->request.url);
    transfer->handler.onBody(std::move(transfer->body));
    transfer->handler.onComplete({}, false);
  }
  drop(transfer->request.id);
}

void HttpEngine::run() {
  pthread_setname_np(pthread_self(), "rn-network");
  auto *multi = static_cast<CURLM *>(multi_);
  while (true) {
    std::deque<std::function<void()>> tasks;
    {
      std::lock_guard lock(mutex_);
      if (stopping_) return;
      tasks.swap(tasks_);
    }
    for (const auto &task : tasks) task();
    int running = 0;
    curl_multi_perform(multi, &running);
    int pending = 0;
    while (const CURLMsg *message = curl_multi_info_read(multi, &pending)) {
      if (message->msg == CURLMSG_DONE) finish(message->easy_handle, message->data.result);
    }
    curl_multi_poll(multi, nullptr, 0, kPollTimeoutMs, nullptr);
  }
}

} // namespace rntizen
