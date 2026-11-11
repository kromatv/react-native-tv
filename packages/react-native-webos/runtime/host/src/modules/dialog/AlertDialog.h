#pragma once

#include "../../input/RemoteInput.h"

#include <Evas.h>

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace rntizen {

/// One button of an alert, and the key `onClose` reports when it is pressed.
struct AlertButton {
  std::string text;
  int key;
};

/// What `Alert.alert` asks for. `onClose` receives the pressed button's key,
/// or nullopt when Back dismissed a cancelable alert.
struct AlertRequest {
  std::string title;
  std::string message;
  std::vector<AlertButton> buttons;
  bool cancelable;
  std::function<void(std::optional<int> buttonKey)> onClose;
};

/// A native alert drawn over the app, driven by the remote: Left and Right
/// move between the buttons, Select presses one, Back dismisses a cancelable
/// alert. Alerts asked for while one is shown wait their turn. Main thread only.
class AlertDialog final {
 public:
  explicit AlertDialog(Evas *evas);
  ~AlertDialog();

  AlertDialog(const AlertDialog &) = delete;
  AlertDialog &operator=(const AlertDialog &) = delete;

  void show(AlertRequest request, int screenWidth, int screenHeight);
  bool isShown() const;

  /// Takes a remote event while an alert is shown.
  void handle(const RemoteEvent &event);

 private:
  void present();
  void layout();
  void focus(std::size_t index);
  void close(std::optional<int> buttonKey);
  void clear();

  Evas *const evas_;
  Evas_Textblock_Style *style_;
  int screenWidth_{0};
  int screenHeight_{0};
  std::deque<AlertRequest> pending_;
  std::optional<AlertRequest> current_;
  std::size_t focused_{0};
  Evas_Object *scrim_{nullptr};
  Evas_Object *panel_{nullptr};
  Evas_Object *text_{nullptr};
  std::vector<Evas_Object *> buttonBackgrounds_;
  std::vector<Evas_Object *> buttonLabels_;
};

} // namespace rntizen
