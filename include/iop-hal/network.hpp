#ifndef IOP_DRIVER_NETWORK_HPP
#define IOP_DRIVER_NETWORK_HPP

#include "iop-hal/response.hpp"
#include "iop-hal/wifi.hpp"
#include "iop-hal/log.hpp"

#include <vector>

namespace iop {
extern iop_hal::Wifi wifi;

/// Hook to schedule the next firmware binary update (_must not_ actually update, but only use it to schedule an update for the next loop run)
///
/// It's called by `iop::Network` whenever the server sets the `LAST_VERSION` HTTP header, to a value that isn't the MD5 of the current firmware binary.
class UpdateHook {
public:
  using UpdateScheduler = void (*) ();
  UpdateScheduler schedule;

  constexpr explicit UpdateHook(UpdateScheduler scheduler) noexcept : schedule(scheduler) {}

  /// No-Op
  static void defaultHook() noexcept;
};

enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  PATCH,
  DELETE,
  CONNECT,
  OPTIONS,
};

/// General higher level HTTPs client made to interact with IoP's server
/// Its purposes are security, good error reporting, no UB possible and ergonomy, in that order.
///
/// Higher level than the simple HTTP client network abstraction, but still focused on the HTTP(S) protocol
///
/// `iop::Network::setUpdateHook` to set the hook that is called to schedule updates.
class Network {
  Log logger_;
  StaticString uri_;

public:
  Network(StaticString uri) noexcept;

  static auto setup() noexcept -> void;
  auto uri() const noexcept -> StaticString { return this->uri_; }
  auto endpoint(StaticString path) const noexcept -> std::string { return this->uri_.toString() + path.toString(); }

  /// Sets new firmware update hook for this. Very useful to support updates
  /// reported by the network (LAST_VERSION header different than current
  /// sketch hash) Default is a noop
  static void setUpdateHook(UpdateHook scheduler) noexcept;
  /// Removes current hook, replaces for default one (noop)
  static auto takeUpdateHook() noexcept -> UpdateHook;

  /// Disconnects from WiFi
  static void disconnect() noexcept;
  /// Checks if WiFi connection is available (doesn't ensure WiFi actually has internet connection)
  static auto isConnected() noexcept -> bool;

  /// Sends an HTTP post that is authenticated to the monitor server.
  auto httpPost(std::string_view token, StaticString path, std::string_view data) noexcept -> iop_hal::Response;

  /// Sends an HTTP post that is not authenticated to the monitor server (used for authentication).
  auto httpPost(StaticString path, std::string_view data) noexcept -> iop_hal::Response;

  /// Sends an HTTP get that is authenticated to the monitor server (used for authentication).
  auto httpGet(StaticString path, std::string_view token, std::string_view data) noexcept -> iop_hal::Response;

  /// Sends a custom HTTP request that may be authenticated to the monitor server (primitive used by higher level methods)
  auto httpRequest(HttpMethod method, const std::optional<std::string_view> &token, StaticString path, const std::optional<std::string_view> &data) noexcept -> iop_hal::Response;

  /// Fetches firmware update from the network
  auto update(StaticString path, std::string_view token) noexcept -> iop_hal::UpdateStatus;

  /// Extracts a network status from the raw response
  auto codeToString(int code) const noexcept -> std::string;

  auto logger() noexcept -> Log &;
};

} // namespace iop
#endif