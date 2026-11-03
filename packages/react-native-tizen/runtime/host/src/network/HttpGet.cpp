#include "HttpGet.h"

#include <curl/curl.h>

#include <memory>
#include <stdexcept>

namespace rntizen {

namespace {

constexpr long kTimeoutSeconds = 120;

std::size_t appendBody(char *data, std::size_t size, std::size_t count, void *body) {
  static_cast<std::string *>(body)->append(data, size * count);
  return size * count;
}

} // namespace

std::string httpGet(const std::string &url) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
  if (!curl) throw std::runtime_error("curl_easy_init failed");
  std::string body;
  curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, appendBody);
  curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, kTimeoutSeconds);
  curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
  const auto result = curl_easy_perform(curl.get());
  if (result != CURLE_OK) throw std::runtime_error("Cannot reach " + url + ": " + curl_easy_strerror(result));
  long status = 0;
  curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
  if (status != 200) throw std::runtime_error(url + " answered " + std::to_string(status) + ":\n" + body);
  return body;
}

} // namespace rntizen
