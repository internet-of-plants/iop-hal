
#include "iop/wifi.hpp"
#include "iop/log.hpp"

namespace driver {
Wifi::Wifi() noexcept {}
Wifi::~Wifi() noexcept {}
StationStatus Wifi::status() const noexcept {
    return StationStatus::GOT_IP;
}
void Wifi::disconnectFromAccessPoint() const noexcept {}
std::pair<iop::NetworkName, iop::NetworkPassword> Wifi::credentials() const noexcept {
  IOP_TRACE()
  iop::NetworkName ssid;
  ssid.fill('\0');
  memcpy(ssid.data(), "SSID", 4);
  
  iop::NetworkPassword psk;
  psk.fill('\0');
  memcpy(psk.data(), "PSK", 3);
  
  return std::make_pair(ssid, psk);
}
std::string Wifi::ourAccessPointIp() const noexcept {
    return "127.0.0.1";
}
void Wifi::enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept { (void) ssid; (void) psk; }
auto Wifi::disableOurAccessPoint() const noexcept -> void {}
bool Wifi::connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept { (void) ssid; (void) psk; return true; }
void Wifi::setup() noexcept {}
void Wifi::setMode(WiFiMode mode) const noexcept { (void) mode; }
void Wifi::onConnect(std::function<void()> f) noexcept { (void) f; }
}