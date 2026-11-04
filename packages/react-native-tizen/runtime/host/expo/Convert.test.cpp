#include "Convert.h"

#include "ExpoModuleRegistry.h"
#include "ExpoTestRuntime.test.h"
#include "ModuleDefinition.h"
#include "SharedObjectRegistry.h"

#include <doctest/doctest.h>

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rntizen::expo {

namespace {

using facebook::jsi::JSError;
using facebook::jsi::Value;
using testing::ExpoTestRuntime;

class Marble final : public SharedObject {};
class Pebble final : public SharedObject {};
class Unregistered final : public SharedObject {};

class ConvertTestModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ConvertTestModule")
        .sharedClass(ClassDefinition<Marble>("Marble").constructor([] { return std::make_shared<Marble>(); }))
        .sharedClass(ClassDefinition<Pebble>("Pebble").constructor([] { return std::make_shared<Pebble>(); }));
  }
};

const bool registered = registerExpoModuleType<ConvertTestModule>("ConvertTestModule");

template <typename T>
T from(ExpoTestRuntime &expo, const std::string &source) {
  return Convert<T>::fromJs(expo.runtime(), expo.eval(source));
}

template <typename T>
std::string message(ExpoTestRuntime &expo, const std::string &source) {
  try {
    from<T>(expo, source);
  } catch (const JSError &error) {
    return error.getMessage();
  }
  return "no error";
}

template <typename T>
std::string json(ExpoTestRuntime &expo, const T &value) {
  auto &runtime = expo.runtime();
  auto converted = Convert<T>::toJs(runtime, value);
  runtime.global().setProperty(runtime, "__converted", converted);
  return expo.json("__converted");
}

} // namespace

TEST_CASE("Convert passes booleans, numbers and strings, and names what it expected") {
  ExpoTestRuntime expo;
  CHECK(from<bool>(expo, "true"));
  CHECK(json(expo, false) == "false");
  CHECK(message<bool>(expo, "1") == "Expected a boolean");

  CHECK(from<int>(expo, "41.9") == 41);
  CHECK(from<double>(expo, "-0.5") == doctest::Approx(-0.5));
  CHECK(from<std::uint8_t>(expo, "255") == 255);
  CHECK(json(expo, std::int64_t{9007199254740991}) == "9007199254740991");
  CHECK(message<int>(expo, "'3'") == "Expected a number");

  CHECK(from<std::string>(expo, "'h\\u00e9llo \\u2713'") == "h\xc3\xa9llo \xe2\x9c\x93");
  CHECK(json(expo, std::string("caf\xc3\xa9")) == "\"caf\xc3\xa9\"");
  CHECK(message<std::string>(expo, "null") == "Expected a string");
}

TEST_CASE("Convert reads undefined and null as no value, and writes no value as null") {
  ExpoTestRuntime expo;
  CHECK_FALSE(from<std::optional<int>>(expo, "undefined").has_value());
  CHECK_FALSE(from<std::optional<int>>(expo, "null").has_value());
  CHECK(from<std::optional<int>>(expo, "7") == 7);
  CHECK(message<std::optional<int>>(expo, "'7'") == "Expected a number");
  CHECK(json(expo, std::optional<std::string>()) == "null");
  CHECK(json(expo, std::optional<std::string>("set")) == "\"set\"");
}

TEST_CASE("Convert passes arrays and objects of converted values") {
  ExpoTestRuntime expo;
  CHECK(from<std::vector<std::string>>(expo, "['a', 'b', 'c']") == std::vector<std::string>{"a", "b", "c"});
  CHECK(from<std::vector<int>>(expo, "[]").empty());
  CHECK(message<std::vector<int>>(expo, "({length: 1, 0: 1})") == "Expected an array");
  CHECK(message<std::vector<int>>(expo, "3") == "Expected an array");
  CHECK(message<std::vector<int>>(expo, "[1, 'two']") == "Expected a number");
  CHECK(json(expo, std::vector<double>{1.5, 2}) == "[1.5,2]");

  const auto entries = from<std::map<std::string, int>>(expo, "({one: 1, two: 2})");
  CHECK(entries == std::map<std::string, int>{{"one", 1}, {"two", 2}});
  CHECK(message<std::map<std::string, int>>(expo, "'text'") == "Expected an object");
  CHECK(message<std::map<std::string, int>>(expo, "({one: 'x'})") == "Expected a number");
  CHECK(json(expo, std::map<std::string, std::string>{{"key", "value"}}) == R"({"key":"value"})");
}

TEST_CASE("Convert passes any JavaScript value as folly::dynamic") {
  ExpoTestRuntime expo;
  const auto value = from<folly::dynamic>(expo, "({list: [1, 'two', null], flag: true})");
  CHECK(value["list"][1].asString() == "two");
  CHECK(value["flag"].asBool());
  CHECK(json(expo, folly::dynamic(folly::dynamic::object("nested", folly::dynamic::array(1, 2)))) == R"({"nested":[1,2]})");
}

TEST_CASE("Convert reads bytes from any typed array, DataView or ArrayBuffer, at its own offset") {
  ExpoTestRuntime expo;
  CHECK(from<Bytes>(expo, "new Uint8Array([104, 105])").data == "hi");
  CHECK(from<Bytes>(expo, "new Uint8Array([0, 65, 66, 67, 0]).subarray(1, 4)").data == "ABC");
  CHECK(from<Bytes>(expo, "new DataView(new Uint8Array([1, 2, 3]).buffer, 1, 1)").data == std::string(1, '\x02'));
  CHECK(from<Bytes>(expo, "new Int16Array([258]).buffer").data == std::string("\x02\x01", 2));
  CHECK(from<Bytes>(expo, "new ArrayBuffer(0)").data.empty());
  CHECK(from<Bytes>(expo, "({buffer: new Uint8Array([7, 8, 9]).buffer, byteOffset: 1, byteLength: 2})").data == "\x08\x09");
  CHECK(Convert<Bytes>::matches(expo.runtime(), expo.eval("new Uint8Array(1)")));
  CHECK_FALSE(Convert<Bytes>::matches(expo.runtime(), expo.eval("'text'")));
}

TEST_CASE("Convert refuses what only looks like a view of bytes") {
  ExpoTestRuntime expo;
  constexpr auto expected = "Expected a typed array or an ArrayBuffer";
  CHECK(message<Bytes>(expo, "'text'") == expected);
  CHECK(message<Bytes>(expo, "({})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: {}, byteOffset: 0, byteLength: 1})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: 'x', byteLength: 1})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: 2, byteLength: 8})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: 5, byteLength: 0})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: -1, byteLength: 1})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: 0, byteLength: -1})") == expected);
  CHECK(message<Bytes>(expo, "({buffer: new ArrayBuffer(4), byteOffset: 0, byteLength: Infinity})") == expected);
}

TEST_CASE("Convert writes bytes as a Uint8Array") {
  ExpoTestRuntime expo;
  CHECK(json(expo, Bytes{std::string("\x00\xff", 2)}) == R"({"0":0,"1":255})");
  auto &runtime = expo.runtime();
  runtime.global().setProperty(runtime, "__bytes", Convert<Bytes>::toJs(runtime, Bytes{}));
  CHECK(expo.eval("__bytes instanceof Uint8Array && __bytes.length === 0").getBool());
}

TEST_CASE("Convert picks the alternative of a two-way variant that matches") {
  ExpoTestRuntime expo;
  using Content = std::variant<std::string, Bytes>;
  CHECK(std::get<std::string>(from<Content>(expo, "'text'")) == "text");
  CHECK(std::get<Bytes>(from<Content>(expo, "new Uint8Array([65])")).data == "A");
  CHECK(message<Content>(expo, "12") == "Expected a typed array or an ArrayBuffer");
  CHECK(json(expo, Content{std::string("text")}) == "\"text\"");
  CHECK(json(expo, Content{Bytes{"B"}}) == R"({"0":66})");
}

TEST_CASE("Convert pairs a shared object with its JavaScript instance, by object or by id") {
  ExpoTestRuntime expo;
  auto &runtime = expo.runtime();
  expo.eval("globalThis.marble = new expo.modules.ConvertTestModule.Marble()");
  const auto native = from<std::shared_ptr<Marble>>(expo, "marble");
  REQUIRE(native != nullptr);
  CHECK(native->sharedObjectId() > 0);
  CHECK(from<std::shared_ptr<Marble>>(expo, "marble.__expo_shared_object_id__") == native);

  runtime.global().setProperty(runtime, "__same", Convert<std::shared_ptr<Marble>>::toJs(runtime, native));
  CHECK(expo.eval("__same === marble").getBool());

  const auto created = std::make_shared<Marble>();
  runtime.global().setProperty(runtime, "__created", Convert<std::shared_ptr<Marble>>::toJs(runtime, created));
  CHECK(expo.eval("__created instanceof expo.modules.ConvertTestModule.Marble").getBool());
  CHECK(created->sharedObjectId() > native->sharedObjectId());
  CHECK(from<std::shared_ptr<Marble>>(expo, "__created") == created);

  CHECK(json(expo, std::shared_ptr<Marble>()) == "null");
}

TEST_CASE("Convert refuses a released object, an object of another class and a plain value") {
  ExpoTestRuntime expo;
  auto &runtime = expo.runtime();
  constexpr auto expected = "Expected a live shared object of the right class";
  expo.eval("globalThis.pebble = new expo.modules.ConvertTestModule.Pebble()");
  CHECK(message<std::shared_ptr<Marble>>(expo, "pebble") == expected);
  CHECK(message<std::shared_ptr<Marble>>(expo, "({})") == expected);
  CHECK(message<std::shared_ptr<Marble>>(expo, "'marble'") == expected);
  CHECK(message<std::shared_ptr<Marble>>(expo, "123456789") == expected);
  expo.eval("globalThis.released = new expo.modules.ConvertTestModule.Marble(); released.release()");
  CHECK(message<std::shared_ptr<Marble>>(expo, "released") == expected);

  try {
    Convert<std::shared_ptr<Unregistered>>::toJs(runtime, std::make_shared<Unregistered>());
    FAIL("a native object with no class converted");
  } catch (const JSError &error) {
    CHECK(error.getMessage() == "This native object has no JavaScript class");
  }
}

} // namespace rntizen::expo
