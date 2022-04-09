#include "driver/string.hpp"

#include <Arduino.h>

namespace iop {
auto StaticString::toString() const noexcept -> std::string {
  std::string msg(this->length(), '\0');
  memcpy_P(&msg.front(), this->asCharPtr(), this->length());
  return msg;
}
auto StaticString::length() const noexcept -> size_t { return strlen_P(this->asCharPtr()); }
}