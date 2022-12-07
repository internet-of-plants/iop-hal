
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

// TODO FIXME: the functions below are buggy
// TODO: handle failure

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
  // TODO FIXME: RCE here, this is super dumb, it's just to make everything work and it takes into account ssid and psk are hardcoded
  // Please do not trust user input in this, please
  //return 0 == std::system((std::string("nmcli dev wifi connect ") + std::string(ssid) + " password " + std::string(psk)).c_str());
  (void) ssid;
  (void) psk;
  return true;
}

void Wifi::disconnectFromAccessPoint() const noexcept {
  //auto code = std::system("nmcli radio wifi off");
  //code = std::system("nmcli radio wifi on");
  //(void) code;
}

void Wifi::enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
  // TODO FIXME: RCE here, this is super dumb, it's just to make everything work and it takes into account ssid and psk are hardcoded
  // Please do not trust user input in this, please
  //auto code = std::system((std::string("nmcli dev wifi hotspot ssid ") + std::string(ssid.substr(0, 32)) + std::string(" password ") + std::string(psk.substr(0, 64))).c_str());
  //(void) code;
  (void) ssid;
  (void) psk;
}

auto Wifi::disableOurAccessPoint() const noexcept -> void {
  //auto code = std::system("nmcli radio wifi off");
  //code = std::system("nmcli radio wifi on");
  //(void) code;
}

void Wifi::setMode(WiFiMode mode) const noexcept {
  //int code;
  switch (mode) {
    case WiFiMode::OFF:
      //code = std::system("nmcli radio wifi off");
      break;
    case WiFiMode::STATION:
    case WiFiMode::ACCESS_POINT:
    case WiFiMode::ACCESS_POINT_AND_STATION:
      //code = std::system("nmcli radio wifi on");
      break;
  }
  //(void) code;
}

// NOOP
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