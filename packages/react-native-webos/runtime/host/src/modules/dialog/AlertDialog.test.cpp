#include "AlertDialog.h"

#include "../../render/RasterTarget.test.h"

#include <doctest/doctest.h>

#include <optional>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

constexpr PixelSize kScreen{.width = 1920, .height = 1080};
constexpr SkColor kFocusedButton = SkColorSetRGB(0x3d, 0x5a, 0xfe);
constexpr SkColor kButton = SkColorSetRGB(0x2d, 0x34, 0x47);
constexpr SkColor kPanel = SkColorSetRGB(0x1f, 0x24, 0x33);
constexpr int kNeutral = -3;
constexpr int kNegative = -2;
constexpr int kPositive = -1;
constexpr int kButtonRowY = 640;
constexpr int kFirstOfThreeX = 620;
constexpr int kButtonPitch = 284;

constexpr RemoteEvent kLeft{.eventType = "left", .keyAction = 1, .isBack = false};
constexpr RemoteEvent kRight{.eventType = "right", .keyAction = 1, .isBack = false};
constexpr RemoteEvent kSelect{.eventType = "select", .keyAction = 1, .isBack = false};
constexpr RemoteEvent kBack{.eventType = "back", .keyAction = 1, .isBack = true};

struct Alerts {
  Scene scene{kScreen};
  AlertDialog dialog{scene};
  std::vector<std::string> closed;

  AlertRequest request(const std::string &title, std::vector<AlertButton> buttons, bool cancelable = true) {
    return {
        .title = title,
        .message = "This cannot be undone.",
        .buttons = std::move(buttons),
        .cancelable = cancelable,
        .onClose = [this, title](std::optional<int> key) {
          closed.push_back(title + ":" + (key ? std::to_string(*key) : "dismissed"));
        },
    };
  }

  static std::vector<AlertButton> threeButtons() {
    return {{.text = "Later", .key = kNeutral}, {.text = "Cancel", .key = kNegative}, {.text = "Delete", .key = kPositive}};
  }
};

} // namespace

TEST_CASE_FIXTURE(Alerts, "an alert opens on its last button, and Left and Right move between its buttons without leaving them") {
  dialog.show(request("Delete?", threeButtons()));
  CHECK(dialog.isShown());

  dialog.handle(kRight);
  dialog.handle(kSelect);
  CHECK(closed == std::vector<std::string>{"Delete?:-1"});
  CHECK_FALSE(dialog.isShown());

  dialog.show(request("Again?", threeButtons()));
  dialog.handle(kLeft);
  dialog.handle(kLeft);
  dialog.handle(kLeft);
  dialog.handle(kSelect);
  CHECK(closed.back() == "Again?:-3");

  dialog.show(request("Middle?", threeButtons()));
  dialog.handle(kLeft);
  dialog.handle(kLeft);
  dialog.handle(kRight);
  dialog.handle(kSelect);
  CHECK(closed.back() == "Middle?:-2");
}

TEST_CASE_FIXTURE(Alerts, "Back dismisses a cancelable alert and leaves any other open") {
  dialog.show(request("Modal", {{.text = "OK", .key = kPositive}}, false));
  dialog.handle(kBack);
  CHECK(dialog.isShown());
  CHECK(closed.empty());
  dialog.handle(kSelect);
  CHECK(closed == std::vector<std::string>{"Modal:-1"});

  dialog.show(request("Cancelable", {{.text = "OK", .key = kPositive}}));
  dialog.handle(kBack);
  CHECK_FALSE(dialog.isShown());
  CHECK(closed.back() == "Cancelable:dismissed");
}

TEST_CASE_FIXTURE(Alerts, "an alert reacts to a key's release only, and to no other key") {
  dialog.show(request("Keys", threeButtons()));

  dialog.handle({.eventType = "longSelect", .keyAction = 0, .isBack = false});
  dialog.handle({.eventType = "select", .keyAction = 0, .isBack = false});
  dialog.handle({.eventType = "up", .keyAction = 1, .isBack = false});
  dialog.handle({.eventType = "playPause", .keyAction = 1, .isBack = false});

  CHECK(dialog.isShown());
  CHECK(closed.empty());
}

TEST_CASE_FIXTURE(Alerts, "alerts asked for while one is shown wait their turn, each closing before the next opens") {
  dialog.show(request("First", {{.text = "OK", .key = kPositive}}));
  dialog.show(request("Second", {{.text = "No", .key = kNegative}, {.text = "Yes", .key = kPositive}}));
  dialog.show(request("Third", {}));

  dialog.handle(kSelect);
  CHECK(closed == std::vector<std::string>{"First:-1"});
  CHECK(dialog.isShown());
  dialog.handle(kLeft);
  dialog.handle(kSelect);
  CHECK(closed.back() == "Second:-2");
  CHECK(dialog.isShown());
  dialog.handle(kSelect);
  CHECK(dialog.isShown());
  dialog.handle(kBack);

  CHECK(closed == std::vector<std::string>{"First:-1", "Second:-2", "Third:dismissed"});
  CHECK_FALSE(dialog.isShown());
}

TEST_CASE_FIXTURE(Alerts, "an alert shown from another alert's close opens once that one is gone") {
  auto first = request("First", {{.text = "OK", .key = kPositive}});
  first.onClose = [this](std::optional<int>) {
    closed.push_back("First");
    dialog.show(request("Chained", {{.text = "OK", .key = kPositive}}));
  };
  dialog.show(std::move(first));

  dialog.handle(kSelect);
  CHECK(dialog.isShown());
  dialog.handle(kSelect);

  CHECK(closed == std::vector<std::string>{"First", "Chained:-1"});
  CHECK_FALSE(dialog.isShown());
}

TEST_CASE_FIXTURE(Alerts, "an alert dims the app, draws its panel and buttons, and lights the focused button") {
  testing::RasterTarget target(kScreen);
  scene.render();

  dialog.show(request("Delete the recording?", threeButtons()));
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());

  CHECK(target.at(10, 10) == SK_ColorBLACK);
  CHECK(target.at(1400, 480) == kPanel);
  CHECK(target.at(kFirstOfThreeX, kButtonRowY) == kButton);
  CHECK(target.at(kFirstOfThreeX + kButtonPitch, kButtonRowY) == kButton);
  CHECK(target.at(kFirstOfThreeX + 2 * kButtonPitch, kButtonRowY) == kFocusedButton);

  scene.render();
  dialog.handle(kLeft);
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(kFirstOfThreeX + kButtonPitch, kButtonRowY) == kFocusedButton);
  CHECK(target.at(kFirstOfThreeX + 2 * kButtonPitch, kButtonRowY) == kButton);

  scene.render();
  dialog.handle(kSelect);
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(1400, 480) == SK_ColorBLACK);
}

TEST_CASE("an alert still shown when its dialog is destroyed leaves the scene without closing") {
  Scene scene(kScreen);
  testing::RasterTarget target(kScreen);
  bool closed = false;
  {
    AlertDialog dialog(scene);
    dialog.show({.title = "Open", .message = "", .buttons = {}, .cancelable = true, .onClose = [&closed](std::optional<int>) {
                   closed = true;
                 }});
  }

  scene.drawInto(target.canvas());

  CHECK_FALSE(closed);
  CHECK(target.at(960, 540) == SK_ColorBLACK);
}

} // namespace rnwebos
