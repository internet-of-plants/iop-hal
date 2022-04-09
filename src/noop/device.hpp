#include "iop/device.hpp"

namespace driver {
auto Device::syncNTP() const noexcept -> void {}
auto Device::platform() const noexcept -> ::iop::StaticString { return IOP_STR("NOOP"); }
auto Device::vcc() const noexcept -> uint16_t { return UINT16_MAX; }
auto Device::availableStorage() const noexcept -> uintmax_t { return 1000000; }
auto Device::deepSleep(const uintmax_t seconds) const noexcept -> void { (void) seconds; }
auto Device::firmwareMD5() const noexcept -> iop::MD5Hash & { static iop::MD5Hash hash; hash.fill('\0'); hash = { 'M', 'D', '5', '\0' }; return hash; }
auto Device::macAddress() const noexcept -> iop::MacAddress & { static iop::MacAddress mac; mac.fill('\0'); mac = { 'M', 'A', 'C', '\0' }; return mac; }
}