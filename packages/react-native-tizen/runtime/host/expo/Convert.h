#pragma once

#include "SharedObject.h"

#include <folly/dynamic.h>
#include <jsi/JSIDynamic.h>
#include <jsi/jsi.h>

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <variant>
#include <vector>

namespace rntizen::expo {

/// Bytes JavaScript receives as a Uint8Array and may pass as any typed array or ArrayBuffer.
struct Bytes {
  std::string data;
};

/// An error JavaScript sees with Expo's `code` property set, as a CodedError.
class CodedError : public std::runtime_error {
 public:
  CodedError(std::string code, const std::string &message) : std::runtime_error(message), code_(std::move(code)) {}

  const std::string &code() const {
    return code_;
  }

 private:
  std::string code_;
};

/// How a C++ type crosses to and from JavaScript in a module's functions,
/// constants and properties: `fromJs` throws a JavaScript TypeError for a value
/// of the wrong type.
template <typename T, typename Enable = void>
struct Convert;

namespace detail {

facebook::jsi::JSError typeError(facebook::jsi::Runtime &runtime, const char *expected);
facebook::jsi::Value bytesToJs(facebook::jsi::Runtime &runtime, const Bytes &bytes);
std::optional<Bytes> bytesFromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value);
facebook::jsi::Value sharedObjectToJs(facebook::jsi::Runtime &runtime, const std::shared_ptr<SharedObject> &object);
std::shared_ptr<SharedObject> sharedObjectFromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value);

} // namespace detail

template <>
struct Convert<bool> {
  static bool fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (!value.isBool()) throw detail::typeError(runtime, "a boolean");
    return value.getBool();
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &, bool value) {
    return facebook::jsi::Value(value);
  }
};

template <typename T>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
struct Convert<T> {
  static T fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (!value.isNumber()) throw detail::typeError(runtime, "a number");
    return static_cast<T>(value.getNumber());
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &, T value) {
    return facebook::jsi::Value(static_cast<double>(value));
  }
};

template <>
struct Convert<std::string> {
  static bool matches(facebook::jsi::Runtime &, const facebook::jsi::Value &value) {
    return value.isString();
  }
  static std::string fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (!value.isString()) throw detail::typeError(runtime, "a string");
    return value.getString(runtime).utf8(runtime);
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::string &value) {
    return facebook::jsi::String::createFromUtf8(runtime, value);
  }
};

template <typename T>
struct Convert<std::optional<T>> {
  static std::optional<T> fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (value.isUndefined() || value.isNull()) return std::nullopt;
    return Convert<T>::fromJs(runtime, value);
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::optional<T> &value) {
    return value ? Convert<T>::toJs(runtime, *value) : facebook::jsi::Value::null();
  }
};

template <typename T>
struct Convert<std::vector<T>> {
  static std::vector<T> fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (!value.isObject() || !value.getObject(runtime).isArray(runtime)) throw detail::typeError(runtime, "an array");
    auto array = value.getObject(runtime).getArray(runtime);
    std::vector<T> items;
    for (std::size_t index = 0, size = array.size(runtime); index < size; ++index) {
      items.push_back(Convert<T>::fromJs(runtime, array.getValueAtIndex(runtime, index)));
    }
    return items;
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::vector<T> &items) {
    facebook::jsi::Array array(runtime, items.size());
    for (std::size_t index = 0; index < items.size(); ++index) {
      array.setValueAtIndex(runtime, index, Convert<T>::toJs(runtime, items[index]));
    }
    return array;
  }
};

template <typename T>
struct Convert<std::map<std::string, T>> {
  static std::map<std::string, T> fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (!value.isObject()) throw detail::typeError(runtime, "an object");
    auto object = value.getObject(runtime);
    auto names = object.getPropertyNames(runtime);
    std::map<std::string, T> entries;
    for (std::size_t index = 0, size = names.size(runtime); index < size; ++index) {
      auto name = names.getValueAtIndex(runtime, index).getString(runtime).utf8(runtime);
      entries.emplace(name, Convert<T>::fromJs(runtime, object.getProperty(runtime, name.c_str())));
    }
    return entries;
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::map<std::string, T> &entries) {
    facebook::jsi::Object object(runtime);
    for (const auto &[name, value] : entries) object.setProperty(runtime, name.c_str(), Convert<T>::toJs(runtime, value));
    return object;
  }
};

template <>
struct Convert<folly::dynamic> {
  static folly::dynamic fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    return facebook::jsi::dynamicFromValue(runtime, value);
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const folly::dynamic &value) {
    return facebook::jsi::valueFromDynamic(runtime, value);
  }
};

template <>
struct Convert<Bytes> {
  static bool matches(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    return detail::bytesFromJs(runtime, value).has_value();
  }
  static Bytes fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    auto bytes = detail::bytesFromJs(runtime, value);
    if (!bytes) throw detail::typeError(runtime, "a typed array or an ArrayBuffer");
    return std::move(*bytes);
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const Bytes &bytes) {
    return detail::bytesToJs(runtime, bytes);
  }
};

template <typename First, typename Second>
struct Convert<std::variant<First, Second>> {
  static std::variant<First, Second> fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    if (Convert<First>::matches(runtime, value)) return Convert<First>::fromJs(runtime, value);
    return Convert<Second>::fromJs(runtime, value);
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::variant<First, Second> &value) {
    return std::visit(
        [&runtime]<typename Alternative>(const Alternative &alternative) {
          return Convert<Alternative>::toJs(runtime, alternative);
        },
        value);
  }
};

template <typename T>
  requires std::is_base_of_v<SharedObject, T>
struct Convert<std::shared_ptr<T>> {
  static std::shared_ptr<T> fromJs(facebook::jsi::Runtime &runtime, const facebook::jsi::Value &value) {
    auto object = std::dynamic_pointer_cast<T>(detail::sharedObjectFromJs(runtime, value));
    if (!object) throw detail::typeError(runtime, "a live shared object of the right class");
    return object;
  }
  static facebook::jsi::Value toJs(facebook::jsi::Runtime &runtime, const std::shared_ptr<T> &object) {
    if (!object) return facebook::jsi::Value::null();
    return detail::sharedObjectToJs(runtime, object);
  }
};

} // namespace rntizen::expo
