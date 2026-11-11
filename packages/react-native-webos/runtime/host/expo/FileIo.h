#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rntizen::expo {

/// The whole file at `path`, or nullopt when it cannot be read.
std::optional<std::string> readFile(const std::filesystem::path &path);

/// Thrown when a file cannot be written; the message names the path.
class FileWriteError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Writes `data` to a temporary file beside `path`, then renames it over
/// `path`, so a reader never sees a partial file. Throws FileWriteError when
/// any step fails.
void writeFileAtomically(const std::filesystem::path &path, std::string_view data);

/// The local path a `file://` URI names, with percent escapes decoded; nullopt
/// for any other scheme. A bare absolute path is returned as is.
std::optional<std::string> localPathOf(const std::string &uri);

} // namespace rntizen::expo
