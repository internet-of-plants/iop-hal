#include "iop-hal/io.hpp"

namespace iop_hal {
namespace io {
void GPIO::setMode(const PinRaw pin, const Mode mode) const noexcept { (void) pin; (void) mode; }
auto GPIO::digitalRead(const PinRaw pin) const noexcept -> Data { (void) pin; return Data::HIGH; }
void GPIO::setInterruptCallback(const PinRaw pin, const InterruptState state, void (*func)()) const noexcept { (void) pin; (void) state; (void) func; }
} // namespace io
} // namespace iop_hal