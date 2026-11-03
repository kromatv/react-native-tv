#include "FileSystemPath.h"

#include "../../FileIo.h"

#include <chrono>
#include <filesystem>

namespace rntizen::expo {

namespace fs = std::filesystem;

namespace {

std::string percentEncoded(const std::string &path) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string encoded;
  for (const char character : path) {
    const auto byte = static_cast<unsigned char>(character);
    if (byte <= 0x20 || byte >= 0x7f || character == '%' || character == '#' || character == '?') {
      encoded += '%';
      encoded += kHex[byte >> 4];
      encoded += kHex[byte & 0x0f];
    } else {
      encoded += character;
    }
  }
  return encoded;
}

std::string normalized(const std::string &path) {
  auto text = fs::path(path).lexically_normal().string();
  while (text.size() > 1 && text.back() == '/') text.pop_back();
  return text;
}

} // namespace

FileSystemPath::FileSystemPath(std::string path) : path_(normalized(path)) {}

std::string FileSystemPath::path() const {
  std::lock_guard lock(mutex_);
  return path_;
}

std::string FileSystemPath::uri() const {
  auto uri = "file://" + percentEncoded(path());
  if (isDirectory() && uri.back() != '/') uri += '/';
  return uri;
}

void FileSystemPath::remove() const {
  const auto current = path();
  if (!fs::exists(current)) throw CodedError("ERR_UNABLE_TO_DELETE", "Unable to delete '" + uri() + "': path does not exist");
  fs::remove_all(current);
}

std::string FileSystemPath::targetIn(const FileSystemPath &destination) const {
  const auto self = fs::path(path());
  if (destination.isDirectory()) return (fs::path(destination.path()) / self.filename()).string();
  if (isDirectory()) throw CodedError("ERR_COPY_OR_MOVE", "Cannot copy or move a directory to a file");
  return destination.path();
}

void FileSystemPath::copyTo(const FileSystemPath &destination, bool overwrite) const {
  const auto target = targetIn(destination);
  if (overwrite && fs::exists(target)) fs::remove_all(target);
  fs::copy(path(), target, fs::copy_options::recursive);
}

void FileSystemPath::moveTo(const FileSystemPath &destination, bool overwrite) {
  const auto target = targetIn(destination);
  if (overwrite && fs::exists(target)) fs::remove_all(target);
  fs::rename(path(), target);
  std::lock_guard lock(mutex_);
  path_ = normalized(target);
}

void FileSystemPath::rename(const std::string &name) {
  const auto target = (fs::path(path()).parent_path() / name).string();
  fs::rename(path(), target);
  std::lock_guard lock(mutex_);
  path_ = normalized(target);
}

std::string pathFromUri(const std::string &uri) {
  auto path = localPathOf(uri);
  if (!path) throw CodedError("ERR_INVALID_URI", "'" + uri + "' is not a local file URI");
  return *path;
}

folly::dynamic pathInfo(const std::string &uri) {
  const auto path = pathFromUri(uri);
  std::error_code error;
  const auto status = fs::status(path, error);
  if (error || !fs::exists(status)) return folly::dynamic::object("exists", false)("isDirectory", nullptr);
  return folly::dynamic::object("exists", true)("isDirectory", fs::is_directory(status));
}

std::optional<double> modificationTime(const std::string &path) {
  std::error_code error;
  const auto time = fs::last_write_time(path, error);
  if (error) return std::nullopt;
  const auto system = std::chrono::clock_cast<std::chrono::system_clock>(time);
  return static_cast<double>(
      std::chrono::duration_cast<std::chrono::milliseconds>(system.time_since_epoch()).count());
}

bool optionFlag(const std::optional<folly::dynamic> &options, const char *name) {
  if (!options || !options->isObject()) return false;
  const auto *value = options->get_ptr(name);
  return value != nullptr && value->isBool() && value->getBool();
}

} // namespace rntizen::expo
