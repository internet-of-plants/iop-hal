#if defined(IOP_LINUX_MOCK) || defined(IOP_LINUX)
#include "posix/linux/wifi.hpp"
#elif defined(IOP_ESP8266)
#include "arduino/esp/esp8266/wifi.hpp"
#elif defined(IOP_ESP32)
#include "arduino/esp/esp32/wifi.hpp"
#elif defined(IOP_NOOP)
#include "noop/wifi.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/log.hpp"

namespace iop_hal {
auto statusToString(const iop_hal::StationStatus status) noexcept -> std::optional<iop::StaticString> {
  IOP_TRACE();

  switch (status) {
  case iop_hal::StationStatus::IDLE:
    return IOP_STR("STATION_IDLE");

  case iop_hal::StationStatus::CONNECTING:
    return IOP_STR("STATION_CONNECTING");

  case iop_hal::StationStatus::WRONG_PASSWORD:
    return IOP_STR("STATION_WRONG_PASSWORD");

  case iop_hal::StationStatus::NO_AP_FOUND:
    return IOP_STR("STATION_NO_AP_FOUND");

  case iop_hal::StationStatus::CONNECT_FAIL:
    return IOP_STR("STATION_CONNECT_FAIL");

  case iop_hal::StationStatus::  GOT_IP:
    return IOP_STR("STATION_GOT_IP");
  }

  return IOP_STR("UNKNWON_STATUS");
  return std::nullopt;
}
}