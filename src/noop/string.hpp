#include "iop-hal/string.hpp"

namespace iop {
// StaticString needs platform specific API, so we just noop it
auto StaticString::toString() const noexcept -> std::string { return ""; }
auto StaticString::length() const noexcept -> size_t { return 0; }
}