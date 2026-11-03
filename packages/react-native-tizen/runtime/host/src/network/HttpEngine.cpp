#include "HttpEngine.h"

#include <curl/curl.h>
#include <pthread.h>

namespace rntizen {

namespace {

constexpr int kPollTimeoutMs = 1000;
// Android and iOS send a User-Agent on every request, and some APIs refuse one
// without it. A User-Agent header from JavaScript still replaces this one.
constexpr const char *kDefaultUserAgent = "react-native-tizen";

std::string trimmed(std::string text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

} // namespace

struct HttpEngine::Transfer {
  HttpRequest request;
  HttpResponseHandler handler;
  CURL *easy{nullptr};
  curl_slist *headerList{nullptr};
  std::string body;
  folly::dynamic headers = folly::dynamic::object();

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
    const std::string line(data, size * count);
    if (line.rfind("HTTP/", 0) == 0) {
      transfer.headers = folly::dynamic::object();
      return size * count;
    }
    const auto colon = line.find(':');
    if (colon == std::string::npos) return size * count;
    const auto name = trimmed(line.substr(0, colon));
    const auto value = trimmed(line.substr(colon + 1));
    if (auto existing = transfer.headers.get_ptr(name)) {
      *existing = existing->asString() + ", " + value;
    } else {
      transfer.headers[name] = value;
    }
    return size * count;
  }
};

HttpEngine::HttpEngine() : multi_(curl_multi_init()), share_(curl_share_init()) {
  curl_share_setopt(static_cast<CURLSH *>(share_), CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  thread_ = std::thread([this] { run(); });
}

HttpEngine::~HttpEngine() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  curl_multi_wakeup(static_cast<CURLM *>(multi_));
  thread_.join();
  transfers_.clear();
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
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> easy(curl_easy_init(), curl_easy_cleanup);
    curl_easy_setopt(easy.get(), CURLOPT_SHARE, share_);
    curl_easy_setopt(easy.get(), CURLOPT_COOKIELIST, "ALL");
    done();
  });
}

void HttpEngine::begin(HttpRequest request, HttpResponseHandler handler) {
  auto transfer = std::make_unique<Transfer>();
  transfer->request = std::move(request);
  transfer->handler = std::move(handler);
  auto *easy = transfer->easy = curl_easy_init();
  const auto &method = transfer->request.method;
  curl_easy_setopt(easy, CURLOPT_URL, transfer->request.url.c_str());
  curl_easy_setopt(easy, CURLOPT_PRIVATE, transfer.get());
  curl_easy_setopt(easy, CURLOPT_SHARE, share_);
  curl_easy_setopt(easy, CURLOPT_COOKIEFILE, "");
  curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(easy, CURLOPT_USERAGENT, kDefaultUserAgent);
  curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, static_cast<long>(transfer->request.timeout.count()));
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, Transfer::appendBody);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, transfer.get());
  curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, Transfer::appendHeader);
  curl_easy_setopt(easy, CURLOPT_HEADERDATA, transfer.get());
  if (method == "HEAD") {
    curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
  } else if (method != "GET") {
    curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.c_str());
  }
  if (transfer->request.body) {
    curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(transfer->request.body->size()));
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, transfer->request.body->c_str());
  }
  for (const auto &[name, value] : transfer->request.headers) {
    transfer->headerList = curl_slist_append(transfer->headerList, (name + ": " + value).c_str());
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

void HttpEngine::finish(void *easy, int result) {
  Transfer *transfer = nullptr;
  curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_PRIVATE, &transfer);
  if (transfer == nullptr) return;
  const auto code = static_cast<CURLcode>(result);
  if (code == CURLE_OK) {
    long status = 0;
    char *effectiveUrl = nullptr;
    curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(static_cast<CURL *>(easy), CURLINFO_EFFECTIVE_URL, &effectiveUrl);
    transfer->handler.onResponse(status, transfer->headers, effectiveUrl != nullptr ? effectiveUrl : transfer->request.url);
    transfer->handler.onBody(std::move(transfer->body));
    transfer->handler.onComplete({}, false);
  } else {
    transfer->handler.onComplete(curl_easy_strerror(code), code == CURLE_OPERATION_TIMEDOUT);
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
    for (auto &task : tasks) task();
    int running = 0;
    curl_multi_perform(multi, &running);
    int pending = 0;
    while (CURLMsg *message = curl_multi_info_read(multi, &pending)) {
      if (message->msg == CURLMSG_DONE) finish(message->easy_handle, message->data.result);
    }
    curl_multi_poll(multi, nullptr, 0, kPollTimeoutMs, nullptr);
  }
}

} // namespace rntizen
