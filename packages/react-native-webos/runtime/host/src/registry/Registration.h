#pragma once

#define RN_TIZEN_CONCAT_INNER(left, right) left##right
#define RN_TIZEN_CONCAT(left, right) RN_TIZEN_CONCAT_INNER(left, right)

/// Runs `expression` once while the library loads, before React Native starts:
/// how a module, a view or a component descriptor adds itself to its registry
/// from its own file, so no central list has to name it.
#define RN_TIZEN_REGISTRATION(expression) \
  [[maybe_unused]] static const bool RN_TIZEN_CONCAT(rnTizenRegistration_, __COUNTER__) = (expression)
