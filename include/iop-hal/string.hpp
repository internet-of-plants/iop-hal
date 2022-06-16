#ifndef IOP_DRIVER_STRING_STATIC_HPP
#define IOP_DRIVER_STRING_STATIC_HPP

#include <string_view>
#include <variant>

#include <array>
#include <string>
#include <cstring>
#include <vector>

class __FlashStringHelper;

#if defined(IOP_ESP8266) || (defined(IOP_NOOP) && defined(ESP8266))
#define INTERNAL_IOP__STRINGIZE_NX(A) #A
#define INTERNAL_IOP__STRINGIZE(A) INTERNAL_IOP__STRINGIZE_NX(A)\

// Arduino's ICACHE_RAM_ATTR
#define IOP_RAM __attribute__((section("\".iram.text." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "." INTERNAL_IOP__STRINGIZE(__COUNTER__) "\"")))
// Arduino's PROGMEM
#define IOP_ROM __attribute__((section( "\".irom.text." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "."  INTERNAL_IOP__STRINGIZE(__COUNTER__) "\"")))

#define IOP_INTERNAL_STORAGE_RAW_N(s,n) (__extension__({static const char __pstr__[] __attribute__((__aligned__(n))) __attribute__((section( "\".irom0.pstr." __FILE__ "." INTERNAL_IOP__STRINGIZE(__LINE__) "."  INTERNAL_IOP__STRINGIZE(__COUNTER__) "\", \"aSM\", @progbits, 1 #"))) = (s); &__pstr__[0];}))
#define IOP_STORAGE_RAW(s) IOP_INTERNAL_STORAGE_RAW_N(s, 4)
#elif defined(IOP_ESP32)
#define _COUNTER_STRINGIFY(COUNTER) #COUNTER
#define _SECTION_ATTR_IMPL(SECTION, COUNTER) __attribute__((section(SECTION "." _COUNTER_STRINGIFY(COUNTER))))
#define IOP_RAM _SECTION_ATTR_IMPL(".iram1", __COUNTER__)
#define IOP_ROM
#define IOP_STORAGE_RAW(x) x
#elif (defined(IOP_NOOP) && !defined(ESP8266)) || defined(IOP_POSIX_MOCK)
#define IOP_RAM
#define IOP_ROM
#define IOP_STORAGE_RAW(x) x
#else
#error "Target not supported"
#endif

#define IOP_STR(string_literal) iop::StaticString(reinterpret_cast<const __FlashStringHelper *>(IOP_STORAGE_RAW(string_literal)))

namespace iop {
/// Allows for possibly destructive operations, like scaping non printable characters
///
/// If the operation needs to modify the data it converts the `std::string_view` into the `std::string` variant
/// Otherwise it can keep storing the reference
using CowString = std::variant<std::string_view, std::string>;

/// Applies FNV hasing to convert a string to a `uint64_t`
auto hashString(std::string_view txt) noexcept -> uint64_t;

/// Checks if a character is ASCII
auto isPrintable(char ch) noexcept -> bool;

/// Checks if all characters are ASCII
auto isAllPrintable(std::string_view txt) noexcept -> bool;

/// If some character isn't ASCII it returns a `std::string` based `CowString` that scapes them
/// If all characters are ASCII it returns a `std::string_view` based `CowString`
///
/// They are scaped by converting the `char` to `uint8_t` and then to string, and surrounding it by `<\` and `>`
///
/// For example the code: `iop::scapeNonPrintable("ABC \1")` returns `std::string("ABC <\1>")`
auto scapeNonPrintable(std::string_view txt) noexcept -> CowString;

using MD5Hash = std::array<char, 32>;
using MacAddress = std::array<char, 17>;
using NetworkName = std::array<char, 32>;
using NetworkPassword = std::array<char, 64>;


/// Helper string that holds a pointer to a string stored in PROGMEM
///
/// It's here to provide a typesafe way to handle IOP_ROM data and to avoid
/// defaulting to the String(__FlashStringHelper*) constructor that allocates implicitly.
///
/// It is not compatible with other strings because it requires 32 bits aligned reads, and
/// things like `strlen` will cause a crash, use `strlen_P`. So we separate them.
///
/// Either call a `_P` functions using `.getCharPtr()` to get the regular
/// pointer. Or construct a String with it using `.get()` so it knows to read
/// from PROGMEM.
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  StaticString() noexcept: str(nullptr) { this->str = nullptr; }
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept: str(str) {}
  
  /// Creates a `std::string` from the compile time string
  auto toString() const noexcept -> std::string;

  // Be careful when calling this function, if you pass a progmem stored `const char *`
  // to a function that expects a RAM stored `const char *`, a hardware exception _may_ happen
  // As IOP_ROM data needs to be read with 32 bits of alignment
  //
  // This has caused trouble in the past and will again.
  auto asCharPtr() const noexcept -> const char * { return reinterpret_cast<const char *>(this->get()); }

  auto get() const noexcept -> const __FlashStringHelper * { return this->str; }

  auto length() const noexcept -> size_t;
};

auto to_view(const std::string& str) -> std::string_view;
auto to_view(const CowString& str) -> std::string_view;
auto to_view(const std::vector<uint8_t>& str) -> std::string_view;
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str, const size_t size) -> std::string_view {
  return std::string_view(str.data(), size);
}
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str) -> std::string_view {
  return to_view(str, strnlen(str.begin(), str.max_size()));
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<const std::array<char, SIZE>> &str, const size_t size) -> std::string_view {
  return to_view(str.get(), size);
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<const std::array<char, SIZE>> &str) -> std::string_view {
  return to_view(str.get());
}
} // namespace iop

#endif