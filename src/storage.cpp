#ifdef IOP_POSIX
#include "cpp17/storage.hpp"
#elif defined(IOP_ESP8266)  
#include "arduino/storage.hpp"
#elif defined(IOP_ESP32)
#include "arduino/storage.hpp"
#elif defined(IOP_NOOP)
#include "noop/storage.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
Storage storage;

auto Storage::read(const uintmax_t address) const noexcept -> std::optional<uint8_t> {
    if (address >= this->size) return std::nullopt;
    return this->read(address);
}

auto Storage::write(const uintmax_t address, uint8_t const val) noexcept -> bool {
    if (address >= this->size) return false;
    this->asMut()[address] = val;
    return true;
}
}