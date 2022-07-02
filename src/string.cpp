#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "cpp17/string.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/string.hpp"
#elif defined(IOP_ESP32)
#include "arduino/string.hpp"
#elif defined(IOP_NOOP)
#include "noop/string.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/panic.hpp"

#include <vector>

namespace iop {
auto hashString(const std::string_view txt) noexcept -> uint64_t {
  IOP_TRACE();
  const uint64_t p = 16777619; // NOLINT cppcoreguidelines-avoid-magic-numbers
  uint64_t hash = 2166136261;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  for (const auto & byte: txt) {
    hash = (hash ^ (uint64_t)byte) * p;
  }

  hash += hash << 13; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 7;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 3;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 17; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 5;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  return hash;
}

auto isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126; // NOLINT cppcoreguidelines-avoid-magic-numbers
}

auto isAllPrintable(const std::string_view txt) noexcept -> bool {
  for (const auto & ch: txt) {
    if (!isPrintable(ch))
      return false;
  }
  return true;
}

auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString {
  if (isAllPrintable(txt))
    return CowString(txt);

  std::string s;
  s.reserve(txt.length());

  for (const auto & ch: txt) {
    if (isPrintable(ch)) {
      s += ch;
    } else {
      s += "<\\";
      s += std::to_string(static_cast<uint8_t>(ch));
      s += ">";
    }
  }
  return CowString(s);
}

auto to_view(const std::string& str) -> std::string_view {
  return str.c_str();
}

auto to_view(const CowString& str) -> std::string_view {
  if (const auto *value = std::get_if<std::string_view>(&str)) {
    return *value;
  } else if (const auto *value = std::get_if<std::string>(&str)) {
    return iop::to_view(*value);
  }
  iop_panic(IOP_STR("Invalid variant types"));
}

auto to_view(const std::vector<uint8_t>& vec) -> std::string_view {
  return std::string_view(reinterpret_cast<const char*>(&vec.front()), vec.size());
}
} // namespace iop
