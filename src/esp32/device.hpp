#include "iop/esp/device.hpp"

//extern "C" int rom_phy_get_vdd33();

namespace driver {
auto Device::platform() const noexcept -> iop::StaticString {
  return IOP_STR("ESP32");
}
auto Device::vcc() const noexcept -> uint16_t {
  return 0; // TODO
}
}