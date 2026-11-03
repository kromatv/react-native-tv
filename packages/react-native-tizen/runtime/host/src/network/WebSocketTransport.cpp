#include "WebSocketTransport.h"

#include "NetworkError.h"

#include <curl/curl.h>

namespace rntizen {

namespace {

constexpr long kConnectTimeoutMs = 10000;

int abortWhenAsked(void *abort, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
  return static_cast<const std::atomic<bool> *>(abort)->load() ? 1 : 0;
}

CURL *handle(void *easy) {
  return static_cast<CURL *>(easy);
}

} // namespace

WebSocketTransport::WebSocketTransport(const std::string &url, const std::atomic<bool> &abort)
    : easy_(curl_easy_init()) {
  auto *easy = handle(easy_);
  curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
  curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 1L);
  // RFC 6455 upgrades an HTTP/1.1 connection: without this, TLS's ALPN can
  // settle on HTTP/2 and libcurl then reads the socket as HTTP/2 frames.
  curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, static_cast<long>(CURL_HTTP_VERSION_1_1));
  curl_easy_setopt(easy, CURLOPT_HTTPPROXYTUNNEL, 1L);
  curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(easy, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
  curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, kConnectTimeoutMs);
  curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, abortWhenAsked);
  curl_easy_setopt(easy, CURLOPT_XFERINFODATA, &abort);
  if (const auto result = curl_easy_perform(easy); result != CURLE_OK) {
    curl_easy_cleanup(easy);
    throw NetworkError(curl_easy_strerror(result));
  }
  curl_socket_t socket = CURL_SOCKET_BAD;
  curl_easy_getinfo(easy, CURLINFO_ACTIVESOCKET, &socket);
  if (socket == CURL_SOCKET_BAD) {
    curl_easy_cleanup(easy);
    throw NetworkError("libcurl kept no socket open");
  }
  socket_ = socket;
}

WebSocketTransport::~WebSocketTransport() {
  curl_easy_cleanup(handle(easy_));
}

int WebSocketTransport::socket() const {
  return socket_;
}

std::optional<std::size_t> WebSocketTransport::receive(char *buffer, std::size_t capacity) {
  std::size_t received = 0;
  const auto result = curl_easy_recv(handle(easy_), buffer, capacity, &received);
  if (result == CURLE_AGAIN) return std::nullopt;
  if (result != CURLE_OK) throw NetworkError(curl_easy_strerror(result));
  return received;
}

std::size_t WebSocketTransport::send(std::string_view bytes) {
  std::size_t sent = 0;
  const auto result = curl_easy_send(handle(easy_), bytes.data(), bytes.size(), &sent);
  if (result == CURLE_AGAIN) return 0;
  if (result != CURLE_OK) throw NetworkError(curl_easy_strerror(result));
  return sent;
}

} // namespace rntizen
