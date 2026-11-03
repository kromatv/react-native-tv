#include "FileSystemFile.h"

#include "../../FileIo.h"

#include "network/Base64.h"
#include "network/Digest.h"

#include <filesystem>
#include <fstream>
#include <map>

namespace rntizen::expo {

namespace fs = std::filesystem;

namespace {

std::string encoding(const std::optional<folly::dynamic> &options) {
  if (!options || !options->isObject()) return "utf8";
  const auto *value = options->get_ptr("encoding");
  return value != nullptr && value->isString() ? value->getString() : "utf8";
}

const std::map<std::string, std::string> &mimeTypes() {
  static const std::map<std::string, std::string> types = {
      {".css", "text/css"},        {".gif", "image/gif"},         {".html", "text/html"},   {".jpeg", "image/jpeg"},
      {".jpg", "image/jpeg"},      {".js", "text/javascript"},    {".json", "application/json"},
      {".mp3", "audio/mpeg"},      {".mp4", "video/mp4"},         {".otf", "font/otf"},     {".pdf", "application/pdf"},
      {".png", "image/png"},       {".svg", "image/svg+xml"},     {".ttf", "font/ttf"},     {".txt", "text/plain"},
      {".webp", "image/webp"},     {".woff", "font/woff"},        {".woff2", "font/woff2"}, {".xml", "application/xml"},
      {".zip", "application/zip"},
  };
  return types;
}

} // namespace

FileSystemFile::FileSystemFile(const std::string &uri)
    : FileSystemPath(pathFromUri(uri)), declaredAsDirectory_(!uri.empty() && uri.back() == '/') {}

bool FileSystemFile::exists() const {
  std::error_code error;
  return fs::is_regular_file(path(), error);
}

void FileSystemFile::validatePath() const {
  if (declaredAsDirectory_) throw CodedError("ERR_WRONG_TYPE", "Tried to create a file with a directory path");
  std::error_code error;
  if (fs::is_directory(path(), error)) throw CodedError("ERR_WRONG_TYPE", "A directory exists at " + uri());
}

void FileSystemFile::create(const std::optional<folly::dynamic> &options) const {
  const auto current = fs::path(path());
  if (!optionFlag(options, "overwrite") && exists()) throw CodedError("ERR_FILE_EXISTS", "A file already exists at " + uri());
  if (optionFlag(options, "intermediates")) fs::create_directories(current.parent_path());
  std::error_code ignored;
  fs::remove(current, ignored);
  std::ofstream file(current, std::ios::binary);
  if (!file) throw CodedError("ERR_UNABLE_TO_CREATE", "Unable to create " + uri());
}

std::string FileSystemFile::read() const {
  auto data = readFile(path());
  if (!data) throw CodedError("ERR_UNABLE_TO_READ", "Unable to read " + uri());
  return std::move(*data);
}

void FileSystemFile::write(const std::variant<std::string, Bytes> &content, const std::optional<folly::dynamic> &options)
    const {
  std::string data;
  if (const auto *text = std::get_if<std::string>(&content)) {
    data = encoding(options) == "base64" ? decodeBase64(*text) : *text;
  } else {
    data = std::get<Bytes>(content).data;
  }
  if (!optionFlag(options, "append")) {
    writeFileAtomically(path(), data);
    return;
  }
  std::ofstream file(path(), std::ios::binary | std::ios::app);
  file.write(data.data(), static_cast<std::streamsize>(data.size()));
  if (!file) throw CodedError("ERR_UNABLE_TO_WRITE", "Unable to write " + uri());
}

double FileSystemFile::size() const {
  std::error_code error;
  const auto bytes = fs::file_size(path(), error);
  return error ? 0 : static_cast<double>(bytes);
}

std::optional<std::string> FileSystemFile::md5() const {
  auto data = readFile(path());
  if (!data) return std::nullopt;
  return md5Hex(*data);
}

std::optional<std::string> FileSystemFile::type() const {
  auto extension = fs::path(path()).extension().string();
  for (auto &character : extension) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  auto found = mimeTypes().find(extension);
  if (found == mimeTypes().end()) return std::nullopt;
  return found->second;
}

folly::dynamic FileSystemFile::info(const std::optional<folly::dynamic> &options) const {
  if (!exists()) return folly::dynamic::object("exists", false)("uri", uri());
  folly::dynamic info = folly::dynamic::object("exists", true)("uri", uri())("size", size());
  if (auto time = modificationTime(path())) info["modificationTime"] = *time;
  if (optionFlag(options, "md5")) {
    if (auto digest = md5()) info["md5"] = *digest;
  }
  return info;
}

ClassDefinition<FileSystemFile> fileClass() {
  using File = FileSystemFile;
  using Content = std::variant<std::string, Bytes>;
  using Options = std::optional<folly::dynamic>;
  return ClassDefinition<File>("FileSystemFile")
      .constructor([](std::string uri) { return std::make_shared<File>(uri); })
      .function("validatePath", [](File &file) { file.validatePath(); })
      .asyncFunction("text", [](File &file) { return file.read(); })
      .function("textSync", [](File &file) { return file.read(); })
      .asyncFunction("base64", [](File &file) { return encodeBase64(file.read()); })
      .function("base64Sync", [](File &file) { return encodeBase64(file.read()); })
      .asyncFunction("bytes", [](File &file) { return Bytes{file.read()}; })
      .function("bytesSync", [](File &file) { return Bytes{file.read()}; })
      .function("info", [](File &file, Options options) { return file.info(options); })
      .asyncFunction("write", [](File &file, Content content, Options options) { file.write(content, options); })
      .function("writeSync", [](File &file, Content content, Options options) { file.write(content, options); })
      .property("size", [](File &file) { return file.size(); })
      .property("md5", [](File &file) { return file.md5(); })
      .property("modificationTime", [](File &file) { return modificationTime(file.path()); })
      .property("lastModified", [](File &file) { return modificationTime(file.path()); })
      .property("creationTime", [](File &) { return std::optional<double>(); })
      .property("type", [](File &file) { return file.type(); })
      .function("delete", [](File &file) { file.remove(); })
      .property("exists", [](File &file) { return file.exists(); })
      .function("create", [](File &file, Options options) { file.create(options); })
      .asyncFunction("copy", [](File &file, std::shared_ptr<FileSystemPath> to, Options options) {
        file.copyTo(*to, optionFlag(options, "overwrite"));
      })
      .function("copySync", [](File &file, std::shared_ptr<FileSystemPath> to, Options options) {
        file.copyTo(*to, optionFlag(options, "overwrite"));
      })
      .asyncFunction("move", [](File &file, std::shared_ptr<FileSystemPath> to, Options options) {
        file.moveTo(*to, optionFlag(options, "overwrite"));
      })
      .function("moveSync", [](File &file, std::shared_ptr<FileSystemPath> to, Options options) {
        file.moveTo(*to, optionFlag(options, "overwrite"));
      })
      .function("rename", [](File &file, std::string name) { file.rename(name); })
      .property("uri", [](File &file) { return file.uri(); });
}

} // namespace rntizen::expo
