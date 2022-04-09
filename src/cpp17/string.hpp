#include "iop/string.hpp"

namespace iop {
auto StaticString::toString() const noexcept -> std::string { return this->asCharPtr(); }
auto StaticString::length() const noexcept -> size_t { return strlen(this->asCharPtr()); }
}