#include "FileSystemFile.h"

#include <filesystem>

namespace rntizen::expo {

namespace fs = std::filesystem;

namespace {

std::string childPath(const FileSystemDirectory &directory, const std::string &name) {
  return (fs::path(directory.path()) / name).string();
}

} // namespace

FileSystemDirectory::FileSystemDirectory(const std::string &uri) : FileSystemPath(pathFromUri(uri)) {}

bool FileSystemDirectory::exists() const {
  std::error_code error;
  return fs::is_directory(path(), error);
}

void FileSystemDirectory::create(const std::optional<folly::dynamic> &options) const {
  const auto current = fs::path(path());
  if (fs::exists(current)) {
    if (optionFlag(options, "idempotent") && exists()) return;
    if (!optionFlag(options, "overwrite")) throw CodedError("ERR_DIRECTORY_EXISTS", "A file or directory already exists at " + uri());
    fs::remove_all(current);
  }
  std::error_code error;
  if (optionFlag(options, "intermediates")) {
    fs::create_directories(current, error);
  } else {
    fs::create_directory(current, error);
  }
  if (error) throw CodedError("ERR_UNABLE_TO_CREATE", "Unable to create " + uri() + ": " + error.message());
}

std::optional<double> FileSystemDirectory::size() const {
  std::error_code error;
  double total = 0;
  for (fs::recursive_directory_iterator entry(path(), error), end; !error && entry != end; entry.increment(error)) {
    std::error_code sizeError;
    if (entry->is_regular_file(sizeError)) total += static_cast<double>(entry->file_size(sizeError));
  }
  if (error) return std::nullopt;
  return total;
}

folly::dynamic FileSystemDirectory::info() const {
  if (!exists()) return folly::dynamic::object("exists", false)("uri", uri());
  folly::dynamic files = folly::dynamic::array();
  std::error_code error;
  for (fs::directory_iterator entry(path(), error), end; !error && entry != end; entry.increment(error)) {
    files.push_back(entry->path().filename().string());
  }
  folly::dynamic info = folly::dynamic::object("exists", true)("uri", uri())("files", std::move(files));
  if (auto bytes = size(); bytes.has_value()) info["size"] = *bytes;
  if (auto time = modificationTime(path()); time.has_value()) info["modificationTime"] = *time;
  return info;
}

std::vector<folly::dynamic> FileSystemDirectory::listAsRecords() const {
  std::vector<folly::dynamic> records;
  std::error_code error;
  for (fs::directory_iterator entry(path(), error), end; !error && entry != end; entry.increment(error)) {
    std::error_code typeError;
    const bool directory = entry->is_directory(typeError);
    const auto child = entry->path().string();
    records.emplace_back(folly::dynamic::object("isDirectory", directory)(
        "uri", directory ? FileSystemDirectory(child).uri() : FileSystemFile(child).uri()));
  }
  if (error) throw CodedError("ERR_UNABLE_TO_LIST", "Unable to list " + uri() + ": " + error.message());
  return records;
}

ClassDefinition<FileSystemDirectory> directoryClass() {
  using Directory = FileSystemDirectory;
  using Options = std::optional<folly::dynamic>;
  using Path = std::shared_ptr<FileSystemPath>;
  return ClassDefinition<Directory>("FileSystemDirectory")
      .constructor([](const std::string &uri) { return std::make_shared<Directory>(uri); })
      .function("info", [](const Directory &directory) { return directory.info(); })
      .function("validatePath", [](const Directory &) {})
      .function("delete", [](const Directory &directory) { directory.remove(); })
      .property("exists", [](const Directory &directory) { return directory.exists(); })
      .function("create", [](const Directory &directory, const Options &options) { directory.create(options); })
      .asyncFunction("copy", [](const Directory &directory, const Path &to, const Options &options) {
        directory.copyTo(*to, optionFlag(options, "overwrite"));
      })
      .function("copySync", [](const Directory &directory, const Path &to, const Options &options) {
        directory.copyTo(*to, optionFlag(options, "overwrite"));
      })
      .asyncFunction("move", [](Directory &directory, const Path &to, const Options &options) {
        directory.moveTo(*to, optionFlag(options, "overwrite"));
      })
      .function("moveSync", [](Directory &directory, const Path &to, const Options &options) {
        directory.moveTo(*to, optionFlag(options, "overwrite"));
      })
      .function("rename", [](Directory &directory, const std::string &name) { directory.rename(name); })
      .function("listAsRecords", [](const Directory &directory) { return directory.listAsRecords(); })
      .function(
          "createFile",
          [](const Directory &directory, const std::string &name, const std::optional<std::string> &) {
            auto file = std::make_shared<FileSystemFile>(childPath(directory, name));
            file->create(std::nullopt);
            return file;
          })
      .function("createDirectory", [](const Directory &directory, const std::string &name) {
        auto child = std::make_shared<Directory>(childPath(directory, name));
        child->create(std::nullopt);
        return child;
      })
      .property("uri", [](const Directory &directory) { return directory.uri(); })
      .property("size", [](const Directory &directory) { return directory.size(); });
}

} // namespace rntizen::expo
