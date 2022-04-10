#ifndef IOP_DRIVER_PINS_HPP
#define IOP_DRIVER_PINS_HPP

#include <stdint.h>

#define IOP_PIN_RAW(pin) static_cast<uint8_t>(pin)

namespace iop_hal {
using PinRaw = uint8_t;

namespace io {
/// GPIO access mode, they either are read-only, or write-only.
enum class Mode {
  INPUT = 0,
  OUTPUT = 1,
};

/// Data representation of digital GPIO access
enum class Data {
  LOW = 0,
  HIGH = 1,
};

/// Represents a state change for a digital GPIO that can trigger an interrupt.
enum class InterruptState {
  RISING = 1,
  FALLING = 2,
  CHANGE = 3,
};

/// High level, type-safe, abstraction to handle GPIO interaction
/// The idea is to be as cross-platform as possible without losing important features, but it's a work in progress
class GPIO {
public:
  /// Sets pin to specific IO mode
  auto setMode(PinRaw pin, Mode mode) const noexcept -> void;

  /// Makes a digital read from specified pin. If it's a write pin reading from it is unspecified behavior.
  /// Most platforms will just return the last value stored
  ///
  /// GPIO mode needs to be better abstracted
  /// TODO UNSAFE
  auto digitalRead(PinRaw pin) const noexcept -> Data;

  /// Monitors specified pin for the desired state, calling callback when it happens
  ///
  /// The callback _MUST_ be cached in RAM (defined with the IOP_RAM macro), as some platforms can't read data from storage (so no fetching opcodes)
  auto setInterruptCallback(PinRaw pin, InterruptState state, void (*func)()) const noexcept -> void;
};
} // namespace io

extern io::GPIO gpio;
} // namespace iop_hal

#endif