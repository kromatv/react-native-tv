#pragma once

#include <Evas.h>

#include <string>
#include <vector>

namespace rntizen {

/// What a fatal error screen says: a title, the message, and the JavaScript
/// stack as one line per frame.
struct ErrorReport {
  std::string title;
  std::string message;
  std::vector<std::string> stack;
};

/// The red screen a development build shows over the app when JavaScript dies,
/// drawn natively so it works even when React never rendered. Main thread only.
class ErrorScreen final {
 public:
  explicit ErrorScreen(Evas *evas);
  ~ErrorScreen();

  ErrorScreen(const ErrorScreen &) = delete;
  ErrorScreen &operator=(const ErrorScreen &) = delete;

  void show(const ErrorReport &report, int screenWidth, int screenHeight);
  bool isShown() const;

 private:
  Evas *const evas_;
  Evas_Object *background_{nullptr};
  Evas_Object *text_{nullptr};
  Evas_Textblock_Style *style_{nullptr};
};

} // namespace rntizen
