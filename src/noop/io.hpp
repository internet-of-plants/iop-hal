#include "iop-hal/io.hpp"

namespace driver {
namespace io {
void GPIO::setMode(const Pin pin, const Mode mode) const noexcept { (void) pin; (void) mode; }
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data { (void) pin; return Data::HIGH; }
void GPIO::setInterruptCallback(const Pin pin, const InterruptState state, void (*func)()) const noexcept { (void) pin; (void) state; (void) func; }
} // namespace io
} // namespace driver