#include "iop-hal/storage.hpp"

#include <EEPROM.h>

namespace iop_hal {
auto Storage::setup(const uintmax_t size) noexcept -> bool {
    this->size = size;
    // Throws exception on OOM
    EEPROM.begin(size);
    return true;
}
auto Storage::commit() noexcept -> bool {
    return EEPROM.commit();
}
// Can never be nullptr as EEPROM.begin throws on OOM
auto Storage::asRef() const noexcept -> uint8_t const * {
    // getConstDataPtr isn't standard so ESP32 doesn't support it
    // UB if EEPROM has const storage, but since we use the mutable static variable we are ok
    return const_cast<EEPROMClass *>(&EEPROM)->getDataPtr();
}
auto Storage::asMut() noexcept -> uint8_t * {
    return EEPROM.getDataPtr();
}
}