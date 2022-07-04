#ifndef IOP_DRIVER_RESPONSE_HPP
#define IOP_DRIVER_RESPONSE_HPP

#include "iop-hal/string.hpp"

#include <unordered_map>
#include <optional>
#include <vector>

namespace iop {
/// Higher level error reporting. Lower level is handled by core
enum class NetworkStatus {
  IO_ERROR = 0,
  BROKEN_SERVER = 500,
  BROKEN_CLIENT = 400,

  OK = 200,
  UNAUTHORIZED = 401,
};
}

namespace iop_hal{
auto networkStatus(int code) noexcept -> std::optional<iop::NetworkStatus>;

class Payload {
public:
  std::vector<uint8_t> payload;
  Payload() noexcept: payload({}) {}
  explicit Payload(std::vector<uint8_t> data) noexcept: payload(std::move(data)) {}
};

class Response {
  std::unordered_map<std::string, std::string> headers_;
  Payload promise;
  int code_;
  std::optional<iop::NetworkStatus> status_;

public:
  Response(const Payload payload, const iop::NetworkStatus status) noexcept: headers_({}), promise(payload), code_(static_cast<uint8_t>(status)), status_(status) {}
  Response(std::unordered_map<std::string, std::string> headers, const Payload payload, const int code) noexcept: headers_(headers), promise(payload), code_(code), status_(iop_hal::networkStatus(code)) {}
  explicit Response(const int code) noexcept: code_(code), status_(iop_hal::networkStatus(code)) {IOP_TRACE();}
  explicit Response(const iop::NetworkStatus status) noexcept: code_(static_cast<uint8_t>(status)), status_(status) {}
  auto code() const noexcept -> int { return this->code_; }
  auto status() const noexcept -> std::optional<iop::NetworkStatus> { return this->status_; }
  auto header(iop::StaticString key) const noexcept -> std::optional<std::string>;
  auto await() noexcept -> Payload {
    // TODO: make it actually lazy
    return std::move(this->promise);
  }
};
}

#endif