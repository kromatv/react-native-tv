#pragma once

#include "../render/Scene.h"

#include "modules/skparagraph/include/Paragraph.h"

#include <memory>
#include <string>
#include <vector>

namespace rnwebos {

/// What a fatal error screen says: a title, the message, and the JavaScript
/// stack as one line per frame.
struct ErrorReport {
  std::string title;
  std::string message;
  std::vector<std::string> stack;
};

/// The red screen a development build shows over the app when JavaScript dies,
/// drawn natively so it works even when React never rendered. Main thread only.
class ErrorScreen final : public SceneLayer {
 public:
  explicit ErrorScreen(Scene &scene);
  ~ErrorScreen() override;

  ErrorScreen(const ErrorScreen &) = delete;
  ErrorScreen &operator=(const ErrorScreen &) = delete;

  void show(const ErrorReport &report);
  bool isShown() const;

  void draw(SkCanvas &canvas) override;

 private:
  Scene &scene_;
  std::unique_ptr<skia::textlayout::Paragraph> text_;
};

} // namespace rnwebos
