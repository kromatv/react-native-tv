#include "ExpoRuntime.h"

#include "ExpoTestRuntime.test.h"

#include "modules/PlatformModuleRegistry.h"

#include <doctest/doctest.h>

#include <hermes/hermes.h>

#include <memory>
#include <string>

namespace rntizen::expo {

namespace {

using facebook::jsi::Object;
using testing::ExpoTestRuntime;

std::string thrown(ExpoTestRuntime &expo, const std::string &source) {
  return expo.eval("(() => { try { " + source + "; return 'no error'; } catch (e) { return e.message; } })()")
      .asString(expo.runtime())
      .utf8(expo.runtime());
}

std::string uriOf(const char *directory) {
  return "\"file://" + (testing::testAppRoot() / directory).string() + "/\"";
}

} // namespace

TEST_CASE("globalThis.expo answers uuidv4 and uuidv5") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(expo.uuidv4())").getBool());
  CHECK(expo.json("expo.uuidv5('hello', '6ba7b810-9dad-11d1-80b4-00c04fd430c8')") ==
        "\"9342d47a-1bab-5709-9869-c840b2eac501\"");
  CHECK(thrown(expo, "expo.uuidv5()") == "uuidv5 expects a string as argument 1");
  CHECK(thrown(expo, "expo.uuidv5('hello', 4)") == "uuidv5 expects a string as argument 2");
  CHECK(thrown(expo, "expo.uuidv5('hello', 'nope')") == "uuidv5 expects a UUID as its namespace, got 'nope'");
}

TEST_CASE("globalThis.expo gives its version as numbers and the app's directories as file URIs") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("(v => v.version === [v.major, v.minor, v.patch].join('.') && v.major >= 58)(expo.expoModulesCoreVersion)")
            .getBool());
  CHECK(expo.json("expo.cacheDir") == uriOf("cache"));
  CHECK(expo.json("expo.documentsDir") == uriOf("data"));
}

TEST_CASE("globalThis.expo answers the view config of a registered view and null for an unknown one") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("expo.getViewConfig('ExpoImage').validAttributes.source === true").getBool());
  CHECK(expo.json("expo.getViewConfig('ExpoImage').directEventTypes.topLoad") == R"({"registrationName":"onLoad"})");
  CHECK(expo.eval("expo.getViewConfig('ExpoVideo', 'VideoView').validAttributes.player === true").getBool());
  CHECK(expo.json("expo.getViewConfig('NoSuchModule')") == "null");
  CHECK(expo.json("expo.getViewConfig('ExpoVideo', 'NoSuchView')") == "null");
  CHECK(thrown(expo, "expo.getViewConfig(1)") == "getViewConfig expects a string as argument 1");
}

TEST_CASE("globalThis.expo rejects reloadAppAsync, as the host cannot reload") {
  ExpoTestRuntime expo;
  CHECK_THROWS_WITH(
      expo.settle("expo.reloadAppAsync('test')"),
      "ERR_UNAVAILABLE: reloadAppAsync is not supported on Tizen: the host cannot reload yet");
}

TEST_CASE("expo.modules creates a registered module from the host's device profile, once") {
  ModuleContext host;
  host.device.modelName = "Test TV";
  ExpoTestRuntime expo(std::move(host));
  CHECK(expo.json("expo.modules.ExpoTizen.modelName") == "\"Test TV\"");
  CHECK(expo.settle("expo.modules.ExpoDevice.getDeviceTypeAsync()") == "4");
  CHECK(expo.eval("expo.modules.ExpoTizen === expo.modules.ExpoTizen").getBool());
}

TEST_CASE("expo.modules lists the registered modules and refuses to be overwritten") {
  ExpoTestRuntime expo;
  CHECK(expo.eval("Object.keys(expo.modules).includes('ExpoTizen')").getBool());
  CHECK(thrown(expo, "expo.modules.ExpoTizen = {}") == "Cannot override the Expo module 'ExpoTizen'");
}

TEST_CASE("installExpoGlobals keeps the globalThis.expo already installed") {
  ExpoTestRuntime expo;
  expo.eval("expo.marker = 'kept'");
  installExpoGlobals(expo.runtime(), expo.context());
  CHECK(expo.json("expo.marker") == "\"kept\"");
}

TEST_CASE("installExpoRuntime installs globalThis.expo through the ExpoModulesCore TurboModule") {
  auto runtime = facebook::hermes::makeHermesRuntime();
  auto &rt = *runtime;
  const auto factory = findPlatformModule("ExpoModulesCore");
  REQUIRE(factory.has_value());
  ModuleContext host;
  host.device.modelName = "Core TV";
  const auto core = (*factory)(std::make_shared<testing::QueuedInvoker>(), host);
  Object proxy(rt);
  proxy.setProperty(rt, "ExpoModulesCore", Object::createFromHostObject(rt, core));
  rt.global().setProperty(rt, "nativeModuleProxy", proxy);

  installExpoRuntime(rt);
  CHECK(rt.global().hasProperty(rt, "expo"));
  auto model = rt.evaluateJavaScript(std::make_shared<facebook::jsi::StringBuffer>("expo.modules.ExpoTizen.modelName"), "test.js");
  CHECK(model.asString(rt).utf8(rt) == "Core TV");
}

TEST_CASE("installExpoRuntime leaves the runtime without Expo when there is no ExpoModulesCore TurboModule") {
  auto withoutProxy = facebook::hermes::makeHermesRuntime();
  installExpoRuntime(*withoutProxy);
  CHECK_FALSE(withoutProxy->global().hasProperty(*withoutProxy, "expo"));

  auto withoutCore = facebook::hermes::makeHermesRuntime();
  withoutCore->global().setProperty(*withoutCore, "nativeModuleProxy", Object(*withoutCore));
  installExpoRuntime(*withoutCore);
  CHECK_FALSE(withoutCore->global().hasProperty(*withoutCore, "expo"));
}

} // namespace rntizen::expo
