#pragma once

#include "../../ExpoTestRuntime.test.h"

#include <filesystem>
#include <string>

namespace rntizen::expo::testing {

/// A fresh directory of its own under the app's data directory, with
/// `globalThis.FS` (the FileSystem module), `globalThis.root` (the directory's
/// URI, with a trailing slash) and `globalThis.fails(f)` (the `code: message`
/// of what `f` throws, or `no error`) set in `expo`'s runtime.
class FileSystemScope final {
 public:
  FileSystemScope(ExpoTestRuntime &expo, const std::string &name)
      : directory_(testAppRoot() / "data" / name) {
    std::filesystem::remove_all(directory_);
    std::filesystem::create_directories(directory_);
    expo.eval(
        "globalThis.FS = expo.modules.FileSystem;"
        "globalThis.root = 'file://" +
        directory_.string() +
        "/';"
        "globalThis.fails = f => { try { f(); return 'no error'; } catch (e) {"
        " return (e && e.code ? e.code + ': ' : '') + (e && e.message !== undefined ? e.message : String(e)); } };");
  }

  ~FileSystemScope() {
    std::error_code ignored;
    std::filesystem::remove_all(directory_, ignored);
  }

  FileSystemScope(const FileSystemScope &) = delete;
  FileSystemScope &operator=(const FileSystemScope &) = delete;

  const std::filesystem::path &directory() const {
    return directory_;
  }

 private:
  std::filesystem::path directory_;
};

} // namespace rntizen::expo::testing
