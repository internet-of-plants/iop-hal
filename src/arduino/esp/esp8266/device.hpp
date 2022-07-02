#include "esp/device.hpp"

#include "Esp.h"

namespace iop_hal {
auto Device::platform() const noexcept -> iop::StaticString {
  return IOP_STR("ESP8266");
}
auto Device::vcc() const noexcept -> uint16_t {
  return ESP.getVcc();
}
}