#include "FallbackFontManager.h"

#include "include/core/SkFontStyle.h"
#include "include/core/SkStream.h"
#include "include/core/SkTypeface.h"

#include <mutex>
#include <unordered_map>
#include <utility>

namespace rnwebos {

namespace {

class FallbackFontManager final : public SkFontMgr {
 public:
  explicit FallbackFontManager(sk_sp<SkFontMgr> fonts) : fonts_(std::move(fonts)) {}

 protected:
  int onCountFamilies() const override {
    return fonts_->countFamilies();
  }

  void onGetFamilyName(int index, SkString *familyName) const override {
    fonts_->getFamilyName(index, familyName);
  }

  sk_sp<SkFontStyleSet> onCreateStyleSet(int index) const override {
    return fonts_->createStyleSet(index);
  }

  sk_sp<SkFontStyleSet> onMatchFamily(const char familyName[]) const override {
    return fonts_->matchFamily(familyName);
  }

  sk_sp<SkTypeface> onMatchFamilyStyle(const char familyName[], const SkFontStyle &style) const override {
    return fonts_->matchFamilyStyle(familyName, style);
  }

  sk_sp<SkTypeface> onMatchFamilyStyleCharacter(
      const char familyName[],
      const SkFontStyle &style,
      const char *bcp47[],
      int bcp47Count,
      SkUnichar character) const override {
    if (auto typeface = fonts_->matchFamilyStyleCharacter(familyName, style, bcp47, bcp47Count, character)) {
      return typeface;
    }
    const int family = familyWith(character, style);
    if (family < 0) return nullptr;
    return fonts_->createStyleSet(family)->matchStyle(style);
  }

  sk_sp<SkTypeface> onMakeFromData(sk_sp<SkData> data, int ttcIndex) const override {
    return fonts_->makeFromData(std::move(data), ttcIndex);
  }

  sk_sp<SkTypeface> onMakeFromStreamIndex(std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const override {
    return fonts_->makeFromStream(std::move(stream), ttcIndex);
  }

  sk_sp<SkTypeface> onMakeFromStreamArgs(std::unique_ptr<SkStreamAsset> stream, const SkFontArguments &args) const override {
    return fonts_->makeFromStream(std::move(stream), args);
  }

  sk_sp<SkTypeface> onMakeFromFile(const char path[], int ttcIndex) const override {
    return fonts_->makeFromFile(path, ttcIndex);
  }

  sk_sp<SkTypeface> onLegacyMakeTypeface(const char familyName[], SkFontStyle style) const override {
    return fonts_->legacyMakeTypeface(familyName, style);
  }

 private:
  int familyWith(SkUnichar character, const SkFontStyle &style) const {
    std::lock_guard lock(mutex_);
    if (const auto known = families_.find(character); known != families_.end()) return known->second;
    int found = -1;
    const int count = fonts_->countFamilies();
    for (int family = 0; family < count && found < 0; ++family) {
      const auto typeface = fonts_->createStyleSet(family)->matchStyle(style);
      if (typeface && typeface->unicharToGlyph(character) != 0) found = family;
    }
    families_.emplace(character, found);
    return found;
  }

  const sk_sp<SkFontMgr> fonts_;
  mutable std::mutex mutex_;
  mutable std::unordered_map<SkUnichar, int> families_;
};

} // namespace

sk_sp<SkFontMgr> withCharacterFallback(sk_sp<SkFontMgr> fonts) {
  return sk_make_sp<FallbackFontManager>(std::move(fonts));
}

} // namespace rnwebos
