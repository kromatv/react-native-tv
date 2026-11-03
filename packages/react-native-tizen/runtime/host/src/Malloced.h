#pragma once

#include <cstdlib>
#include <memory>

namespace rntizen {

/// Frees what a C library allocated with malloc and left to its caller.
struct MallocDeleter {
  template <typename T>
  void operator()(T *memory) const noexcept {
    std::free(memory);
  }
};

/// Owns what a C library allocated with malloc and left to its caller, such as
/// the string `ecore_imf_context_preedit_string_get` returns.
template <typename T>
using Malloced = std::unique_ptr<T, MallocDeleter>;

} // namespace rntizen
