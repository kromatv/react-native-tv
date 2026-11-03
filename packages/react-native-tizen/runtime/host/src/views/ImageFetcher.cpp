#include "ImageFetcher.h"

#include "../network/Base64.h"
#include "../network/HttpEngine.h"
#include "../threading/MainThread.h"

#include <atomic>
#include <list>
#include <stdexcept>
#include <unordered_map>

namespace rntizen {

class ImageFetch {
 public:
  explicit ImageFetch(std::function<void(FetchedImage)> done) : done_(std::move(done)) {}

  void deliver(FetchedImage image) const {
    done_(std::move(image));
  }

 private:
  std::function<void(FetchedImage)> done_;
};

namespace {

constexpr std::size_t kCacheBytes = 48u << 20;
constexpr std::chrono::milliseconds kTimeout{30000};

class ByteCache {
 public:
  std::shared_ptr<const std::string> find(const std::string &uri) {
    auto found = entries_.find(uri);
    if (found == entries_.end()) return nullptr;
    order_.splice(order_.begin(), order_, found->second.position);
    return found->second.bytes;
  }

  void insert(const std::string &uri, std::shared_ptr<const std::string> bytes) {
    if (bytes->size() > kCacheBytes / 4 || entries_.contains(uri)) return;
    size_ += bytes->size();
    order_.push_front(uri);
    entries_[uri] = {std::move(bytes), order_.begin()};
    while (size_ > kCacheBytes && !order_.empty()) {
      auto oldest = entries_.find(order_.back());
      size_ -= oldest->second.bytes->size();
      entries_.erase(oldest);
      order_.pop_back();
    }
  }

 private:
  struct Entry {
    std::shared_ptr<const std::string> bytes;
    std::list<std::string>::iterator position;
  };

  std::unordered_map<std::string, Entry> entries_;
  std::list<std::string> order_;
  std::size_t size_{0};
};

ByteCache &byteCache() {
  static ByteCache cache;
  return cache;
}

HttpEngine &imageEngine() {
  static auto *engine = new HttpEngine();
  return *engine;
}

int nextRequestId() {
  static std::atomic<int> next{1};
  return next++;
}

void deliverOnMainThread(const std::weak_ptr<ImageFetch> &fetch, FetchedImage image) {
  runOnMainThread([fetch, image = std::move(image)]() mutable {
    if (auto alive = fetch.lock()) alive->deliver(std::move(image));
  });
}

FetchedImage decodeDataUri(const std::string &uri) {
  const auto comma = uri.find(',');
  if (comma == std::string::npos) return {.bytes = nullptr, .error = "Malformed data URI"};
  const auto header = uri.substr(0, comma);
  const auto payload = std::string_view(uri).substr(comma + 1);
  if (header.find(";base64") == std::string::npos) {
    return {.bytes = std::make_shared<const std::string>(payload), .error = {}};
  }
  try {
    return {.bytes = std::make_shared<const std::string>(decodeBase64(payload)), .error = {}};
  } catch (const std::invalid_argument &error) {
    return {.bytes = nullptr, .error = error.what()};
  }
}

void download(const facebook::react::ImageSource &source, const std::weak_ptr<ImageFetch> &fetch) {
  struct Result {
    long status{0};
    std::string body;
  };
  auto result = std::make_shared<Result>();
  const auto uri = source.uri;
  imageEngine().start(
      HttpRequest{.id = nextRequestId(), .method = "GET", .url = uri, .headers = source.headers, .body = {}, .timeout = kTimeout},
      HttpResponseHandler{
          .onResponse = [result](long status, folly::dynamic, std::string) { result->status = status; },
          .onBody = [result](std::string body) { result->body = std::move(body); },
          .onComplete =
              [result, fetch, uri](std::string error, bool) {
                const auto status = static_cast<int>(result->status);
                if (error.empty() && (status < 200 || status >= 300)) error = "HTTP " + std::to_string(status);
                if (!error.empty()) {
                  deliverOnMainThread(fetch, {.bytes = nullptr, .error = error, .status = status});
                  return;
                }
                auto bytes = std::make_shared<const std::string>(std::move(result->body));
                runOnMainThread([fetch, uri, bytes, status] {
                  byteCache().insert(uri, bytes);
                  if (auto alive = fetch.lock()) alive->deliver({.bytes = bytes, .error = {}, .status = status});
                });
              },
      });
}

} // namespace

bool isFetchedUri(const std::string &uri) {
  return uri.starts_with("http://") || uri.starts_with("https://") || uri.starts_with("data:");
}

std::shared_ptr<ImageFetch> fetchImage(const facebook::react::ImageSource &source, std::function<void(FetchedImage)> done) {
  auto fetch = std::make_shared<ImageFetch>(std::move(done));
  const std::weak_ptr<ImageFetch> weak = fetch;
  if (source.uri.starts_with("data:")) {
    deliverOnMainThread(weak, decodeDataUri(source.uri));
  } else if (auto cached = byteCache().find(source.uri)) {
    deliverOnMainThread(weak, {.bytes = std::move(cached), .error = {}, .status = 200});
  } else {
    download(source, weak);
  }
  return fetch;
}

} // namespace rntizen
