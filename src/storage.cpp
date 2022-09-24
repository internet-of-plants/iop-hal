#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
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

namespace iop_hal {
Storage storage;

auto Storage::get(const uintmax_t address) const noexcept -> std::optional<uint8_t> {
    if (address >= this->size) return std::nullopt;
    return this->asRef()[address];
}

auto Storage::set(const uintmax_t address, uint8_t const val) noexcept -> bool {
    if (address >= this->size) return false;
    this->asMut()[address] = val;
    return true;
}
}