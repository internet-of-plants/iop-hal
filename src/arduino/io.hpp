#include "iop-hal/io.hpp"

#include <Arduino.h>
#undef HIGH
#undef LOW

namespace iop_hal {
namespace io {
auto GPIO::setMode(const Pin pin, const Mode mode) const noexcept -> void {
    ::pinMode(static_cast<uint8_t>(pin), static_cast<uint8_t>(mode));
}
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data {
    return ::digitalRead(static_cast<uint8_t>(pin)) ? Data::HIGH : Data::LOW;
}
auto GPIO::setInterruptCallback(const Pin pin, const InterruptState state, void (*func)()) const noexcept -> void {
    ::attachInterrupt(digitalPinToInterrupt(static_cast<uint8_t>(pin)), func, static_cast<uint8_t>(state));
}
} // namespace io
} // namespace iop_hal