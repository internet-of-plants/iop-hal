#ifdef IOP_POSIX
#include "posix/wifi.hpp"
#elif defined(IOP_ESP8266)
#include "esp8266/wifi.hpp"
#elif defined(IOP_ESP32)
#include "esp32/wifi.hpp"
#elif defined(IOP_NOOP)
#include "noop/wifi.hpp"
#else
#error "Target not supported"
#endif

#include "iop-hal/log.hpp"

namespace driver {
auto statusToString(const driver::StationStatus status) noexcept -> std::optional<iop::StaticString> {
  IOP_TRACE();

  switch (status) {
  case driver::StationStatus::IDLE:
    return IOP_STR("STATION_IDLE");

  case driver::StationStatus::CONNECTING:
    return IOP_STR("STATION_CONNECTING");

  case driver::StationStatus::WRONG_PASSWORD:
    return IOP_STR("STATION_WRONG_PASSWORD");

  case driver::StationStatus::NO_AP_FOUND:
    return IOP_STR("STATION_NO_AP_FOUND");

  case driver::StationStatus::CONNECT_FAIL:
    return IOP_STR("STATION_CONNECT_FAIL");

  case driver::StationStatus::  GOT_IP:
    return IOP_STR("STATION_GOT_IP");
  }

  return IOP_STR("UNKNWON_STATUS");
  return std::nullopt;
}
}