#ifndef IOP_DRIVER_WIFI
#define IOP_DRIVER_WIFI

#include "iop-hal/string.hpp"
#include "iop-hal/client.hpp"
#include "iop-hal/update.hpp"
#include <string>
#include <array>
#include <functional>
#include <optional>

namespace iop_hal {
enum class WiFiMode {
  OFF = 0, STATION, ACCESS_POINT, ACCESS_POINT_AND_STATION
};

enum class StationStatus {
  IDLE = 0,
  CONNECTING,
  WRONG_PASSWORD,
  NO_AP_FOUND,
  CONNECT_FAIL,
  GOT_IP,
};
auto statusToString(const iop_hal::StationStatus status) noexcept -> iop::StaticString;

class CertStore;

class Wifi {
  void *client;
public:
  // Initializes wifi configuration, attaches TLS certificates storage to underlying HTTP code
  auto setup() noexcept -> void;

  // TODO: document this

  // TODO: maybe can be removed. Mostly used to check if connected, or to display the status in debugging data
  auto status() const noexcept -> StationStatus;
  // TODO: Could be removed with a little work
  auto setMode(WiFiMode mode) const noexcept -> void;

  // TODO: those next two can be removed by a driver interrupt kind that can handle robust functions
  // So a runtime called at every event-loop. So that we attach a handler that receives the credentials
  // Instead of scheduling the run to then retrieve the credentials to then store then, abstract this from user
  auto credentials() const noexcept -> std::pair<iop::NetworkName, iop::NetworkPassword>;

  auto connectToAccessPoint(std::string_view ssid, std::string_view psk) const noexcept -> bool;
  // TODO: Remove this. This is only used to disconnect after a factory reset, is it needed?
  auto disconnectFromAccessPoint() const noexcept -> void;

  auto enableOurAccessPoint(std::string_view ssid, std::string_view psk) const noexcept -> void;
  auto disableOurAccessPoint() const noexcept -> void;
  auto ourAccessPointIp() const noexcept -> std::string;

  Wifi() noexcept;
  ~Wifi() noexcept;
  Wifi(Wifi &other) noexcept = delete;
  Wifi(Wifi &&other) noexcept;
  auto operator=(Wifi &other) noexcept -> Wifi & = delete;
  auto operator=(Wifi &&other) noexcept -> Wifi &;

  friend HTTPClient;
  friend Update;
};
}


#endif