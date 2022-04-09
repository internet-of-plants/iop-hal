#ifndef IOP_DRIVER_PINS_HPP
#define IOP_DRIVER_PINS_HPP

namespace driver {
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

/// Enumerates the possible GPIO ports for the device
///
/// It gets awkward normalizing GPIO between backends
/// TODO: this is made for NodeMCU2 (IOP_ESP8266), we need to fix cross platform support here
enum class Pin {
  D1 = 5,
  D2 = 4,
  D5 = 14,
  D6 = 12,
  D7 = 13
};
/// Some devices come with a builtin led, which is used to communicate some states, like firmware update handling
/// If the device doesn't have one, this will cause a No-Op
constexpr static Pin LED_BUILTIN = Pin::D2;

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
  auto setMode(Pin pin, Mode mode) const noexcept -> void;

  /// Makes a digital read from specified pin. If it's a write pin reading from it is unspecified behavior.
  /// Most platforms will just return the last value stored
  ///
  /// GPIO mode needs to be better abstracted
  /// TODO UNSAFE
  auto digitalRead(Pin pin) const noexcept -> Data;

  /// Monitors specified pin for the desired state, calling callback when it happens
  ///
  /// The callback _MUST_ be cached in RAM (defined with the IOP_RAM macro), as some platforms can't read data from storage (so no fetching opcodes)
  auto setInterruptCallback(Pin pin, InterruptState state, void (*func)()) const noexcept -> void;
};
} // namespace io

extern io::GPIO gpio;
} // namespace driver

#endif