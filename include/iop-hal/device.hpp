#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "iop-hal/string.hpp"
#include <stdint.h>
#include <array>

namespace driver {
/// High level abstraction to manage and monitor device resources
class Device {
public:
  /// Measures current storage available in the device. Depending on the device it might be flash memory, HDD, SSD, etc.
  auto availableStorage() const noexcept -> uintmax_t;

  /// Measures current VCC usage by the device, some platforms may return UINT16_MAX as they don't have an API for monitoring it.
  auto vcc() const noexcept -> uint16_t;

  /// Puts device in standby mode, pauses all threads, so the device uses almost no energy.
  auto deepSleep(uintmax_t seconds) const noexcept -> void;

  /// Returns a reference to a static buffer containing the firmware's MD5 hash
  auto firmwareMD5() const noexcept -> iop::MD5Hash &;

  /// Returns a reference to a static buffer containing the device's MacAddress
  auto macAddress() const noexcept -> iop::MacAddress &;

  /// Retrieves the device's platform name, as in the driver backend it's using
  auto platform() const noexcept -> iop::StaticString;

  /// Syncronizes internal clock with the network, might be a No-Op depending on the device
  /// Returns true if it succeeds, false if it fails 
  ///
  /// Note: network.cpp might be a more appropriate place for this
  /// TODO: report errors, check for network, etc
  auto syncNTP() const noexcept -> void;
};
extern Device device;
}

#endif