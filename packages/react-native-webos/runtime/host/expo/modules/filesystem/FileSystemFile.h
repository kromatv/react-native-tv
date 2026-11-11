#pragma once

#include "FileSystemPath.h"

namespace rntizen::expo {

/// The native side of expo-file-system's `File`.
class FileSystemFile final : public FileSystemPath {
 public:
  explicit FileSystemFile(const std::string &uri);

  bool isDirectory() const override {
    return false;
  }

  bool exists() const;
  void validatePath() const;
  void create(const std::optional<folly::dynamic> &options) const;
  std::string read() const;
  void write(const std::variant<std::string, Bytes> &content, const std::optional<folly::dynamic> &options) const;
  double size() const;
  std::optional<std::string> md5() const;
  std::optional<std::string> type() const;
  folly::dynamic info(const std::optional<folly::dynamic> &options) const;

 private:
  bool declaredAsDirectory_;
};

/// The native side of expo-file-system's `Directory`.
class FileSystemDirectory final : public FileSystemPath {
 public:
  explicit FileSystemDirectory(const std::string &uri);

  bool isDirectory() const override {
    return true;
  }

  bool exists() const;
  void create(const std::optional<folly::dynamic> &options) const;
  std::optional<double> size() const;
  folly::dynamic info() const;
  std::vector<folly::dynamic> listAsRecords() const;
};

} // namespace rntizen::expo
