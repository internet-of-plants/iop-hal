
#include "iop-hal/wifi.hpp"
#include "iop-hal/log.hpp"

#ifdef IOP_SSL
#include <openssl/ssl.h>
#include <filesystem>
#include <fstream>

#include "iop-hal/panic.hpp"
#include "generated/certificates.hpp"
#endif

namespace iop_hal {
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
void Wifi::setup() noexcept {
#ifdef IOP_SSL
  SSL_library_init();
  SSL_load_error_strings();
  
  const auto path = std::filesystem::temp_directory_path().append("iop-hal-posix-mock-certs-bundle.crt");
  std::ofstream file(path);
  iop_assert(file.is_open(), std::string("Unable to create certs bundle file: ") + path.c_str());
  
  iop_assert(generated::certs_bundle, IOP_STR("Cert Bundle is null, but SSL is enabled"));
  file.write((char*) generated::certs_bundle, static_cast<std::streamsize>(sizeof(generated::certs_bundle)));
  iop_assert(!file.fail(), "Unable to write to certs bundle file");

  file.close();
  iop_assert(!file.fail(), "Unable to close certs bundle file");
#endif
}
void Wifi::setMode(WiFiMode mode) const noexcept { (void) mode; }
void Wifi::onConnect(std::function<void()> f) noexcept { (void) f; }
}