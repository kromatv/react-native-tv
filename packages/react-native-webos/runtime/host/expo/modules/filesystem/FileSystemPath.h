#pragma once

#include "../../ModuleDefinition.h"
#include "../../SharedObject.h"

#include <folly/dynamic.h>

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>

namespace rntizen::expo {

/// A location expo-file-system's File or Directory points to: a local path
/// that `move` and `rename` update. Thread-safe.
class FileSystemPath : public SharedObject {
 public:
  explicit FileSystemPath(const std::string &path);

  std::string path() const;
  virtual bool isDirectory() const = 0;

  std::string uri() const;

  void remove() const;
  void copyTo(const FileSystemPath &destination, bool overwrite) const;
  void moveTo(const FileSystemPath &destination, bool overwrite);
  void rename(const std::string &name);

 private:
  std::filesystem::path targetIn(const FileSystemPath &destination) const;

  mutable std::mutex mutex_;
  std::string path_;
};

/// The local path a JavaScript URI names; throws a CodedError for a URI that
/// is not a local file.
std::string pathFromUri(const std::string &uri);

/// `{ exists, isDirectory }` for the `info(uri)` module function.
folly::dynamic pathInfo(const std::string &uri);

/// Milliseconds since the epoch at which `path` last changed, if it exists.
std::optional<double> modificationTime(const std::string &path);

/// Whether the options object JavaScript passed sets the boolean `name` to true.
bool optionFlag(const std::optional<folly::dynamic> &options, const char *name);

class FileSystemFile;
class FileSystemDirectory;

ClassDefinition<FileSystemFile> fileClass();
ClassDefinition<FileSystemDirectory> directoryClass();

} // namespace rntizen::expo
