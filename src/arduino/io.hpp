#include "iop-hal/io.hpp"

#include <Arduino.h>
#undef HIGH
#undef LOW

namespace iop_hal {
namespace io {
auto GPIO::setMode(const PinRaw pin, const Mode mode) const noexcept -> void {
    ::pinMode(pin, static_cast<uint8_t>(mode));
}
auto GPIO::digitalRead(const PinRaw pin) const noexcept -> Data {
    return ::digitalRead(pin) ? Data::HIGH : Data::LOW;
}
auto GPIO::setInterruptCallback(const PinRaw pin, const InterruptState state, void (*func)()) const noexcept -> void {
    ::attachInterrupt(digitalPinToInterrupt(pin), func, static_cast<uint8_t>(state));
}
} // namespace io
} // namespace iop_hal