
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

namespace iop_hal {
Wifi::Wifi() noexcept {}
Wifi::~Wifi() noexcept {}
void Wifi::setup() noexcept { HTTPClient::setup(); }

// TODO FIXME: the functions below are buggy
// TODO: handle failure

// This is horrible, we should have a better way, this is just a hack to ensure feature parity between targets
// -1 unset, 0 false, 1 true
static auto isConnected = std::atomic<int>(-1);
StationStatus Wifi::status() const noexcept {
  const auto isConn = isConnected.load();
  if (isConn == -1) {
    isConnected.store(1); // We pretend that we are connected so we can properly check for it and unset on failure
    iop_hal::HTTPClient http;
    const auto code = http.begin("https://google.com", [](iop_hal::Session & session) {
      return session.sendRequest("GET", "");
    }).code();
    isConnected.store(code > 0);
  }
  return isConnected.load() == 1 ? StationStatus::GOT_IP : StationStatus::CONNECT_FAIL;
}

void Wifi::onConnect(std::function<void()> f) noexcept {
  IOP_TRACE();
  std::thread([](std::function<void()> f) {
    auto wifi = Wifi();
    iop_hal::HTTPClient http;
    auto lastConnection = std::chrono::system_clock::now();

    while (true) {
      const auto isConn = isConnected.load() == 1;
      if (isConn) {
        std::this_thread::sleep_for(std::chrono::seconds(600));
      }

      const auto conn = http.begin("https://google.com", [](iop_hal::Session & session) {
        return session.sendRequest("GET", "");
      }).code() > 0;

      isConnected.store(conn);
      lastConnection = std::chrono::system_clock::now();

      if (!isConn && conn) {
        f();
      }
      std::this_thread::sleep_for(std::chrono::seconds(60));
    }
  }, std::move(f)).detach();
}

bool Wifi::connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
  // TODO FIXME: RCE here, this is super dumb, it's just to make everything work and it takes into account ssid and psk are hardcoded
  // Please do not trust user input in this, please
  return 0 == std::system((std::string("nmcli dev wifi connect ") + std::string(ssid) + " password " + std::string(psk)).c_str());
}

void Wifi::disconnectFromAccessPoint() const noexcept {
  auto code = std::system("nmcli radio wifi off");
  code = std::system("nmcli radio wifi on");
  (void) code;
}

void Wifi::enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept {
  // TODO FIXME: RCE here, this is super dumb, it's just to make everything work and it takes into account ssid and psk are hardcoded
  // Please do not trust user input in this, please
  auto code = std::system((std::string("nmcli dev wifi hotspot ssid ") + std::string(ssid.substr(0, 32)) + " password " + std::string(psk.substr(0, 64)).c_str());
  (void) code;
}

auto Wifi::disableOurAccessPoint() const noexcept -> void {
  auto code = std::system("nmcli radio wifi off");
  code = std::system("nmcli radio wifi on");
  (void) code;
}

void Wifi::setMode(WiFiMode mode) const noexcept {
  int code;
  switch (mode) {
    case WiFiMode::OFF:
      code = std::system("nmcli radio wifi off");
      break;
    case WiFiMode::STATION:
    case WiFiMode::ACCESS_POINT:
    case WiFiMode::ACCESS_POINT_AND_STATION:
      code = std::system("nmcli radio wifi on");
      break;
  }
  (void) code;
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