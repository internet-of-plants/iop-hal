
#include "iop-hal/wifi.hpp"
#include "iop-hal/log.hpp"
#include "iop-hal/client.hpp"

#include <cstdlib>

#include <thread>
#include <memory>
#include <atomic>

/*
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
*/

#include <iostream>

namespace iop_hal {
Wifi::Wifi() noexcept {}
Wifi::~Wifi() noexcept {}

void Wifi::setup() noexcept {
  HTTPClient::setup();
}

StationStatus Wifi::status() const noexcept {
  static auto running = false;
  if (running) return StationStatus::GOT_IP;
  running = true;

  iop_hal::HTTPClient http;
  const auto code = http.begin("https://google.com", [](iop_hal::Session & session) {
    return session.sendRequest("GET", "");
  }).code();
  if (code <= 0) std::cout << code << std::endl;
  return code > 0 ? StationStatus::GOT_IP : StationStatus::CONNECT_FAIL;
}

bool Wifi::connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
  (void) ssid;
  (void) psk;
  return true;
}

void Wifi::disconnectFromAccessPoint() const noexcept {}

void Wifi::enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
  (void) ssid;
  (void) psk;
}

auto Wifi::disableOurAccessPoint() const noexcept -> void {}

void Wifi::setMode(WiFiMode mode) const noexcept {
  (void) mode;
}

// FIXME TODO: Lies
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

std::string Wifi::ourAccessPointIp() const noexcept { return "127.0.0.1"; }
}
