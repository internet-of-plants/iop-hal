#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "iop-hal/string.hpp"
#include "iop-hal/thread.hpp"
#include <stdint.h>
#include <array>
#include <time.h>

namespace iop_hal {
struct Moment {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    Moment(uint8_t hour, uint8_t minute, uint8_t second) noexcept: hour(hour), minute(minute), second(second) {}

    static auto now() noexcept -> Moment {
      const auto rawtime = time(nullptr);
      const auto *time = localtime(&rawtime);
      return Moment(static_cast<uint8_t>(time->tm_hour), static_cast<uint8_t>(time->tm_min), static_cast<uint8_t>(time->tm_sec));
    }

    auto operator==(const Moment & other) const noexcept -> bool {
        return this->hour == other.hour && this->minute == other.minute && this->second == other.second;
    }
    auto operator< (const Moment & other) const noexcept -> bool {
        return this->hour < other.hour
            || (this->hour == other.hour && this->minute < other.minute)
            || (this->hour == other.hour && this->minute == other.minute && this->second < other.second);
    }
    auto operator> (const Moment & other) const noexcept -> bool { return other < *this; }
    auto operator<=(const Moment & other) const noexcept -> bool { return !(*this > other); }
    auto operator>=(const Moment & other) const noexcept -> bool { return !(*this < other); }
    auto setSecond(uint8_t second) noexcept -> void {
      this->second = second;
    }
};

/// High level abstraction to manage and monitor device resources
class Device {
  iop::time::milliseconds timezone;
public:
  auto setTimezone(const int8_t timezone) noexcept {
    this->timezone = timezone * 3600;
  }

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

  auto now() const noexcept -> Moment {
    return Moment::now();
  }
};
extern Device device;
}

template<>
struct std::hash<iop_hal::Moment> {
    std::size_t operator()(const iop_hal::Moment & moment) const noexcept {
        return static_cast<size_t>(moment.hour) ^ (static_cast<size_t>(moment.minute) << 1) ^ (static_cast<size_t>(moment.second) << 2);
    }
};

#endif
