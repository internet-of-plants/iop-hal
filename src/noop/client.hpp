#include "iop-hal/client.hpp"
#include "iop-hal/network.hpp"

namespace iop { 
auto Network::codeToString(const int code) const noexcept -> std::string {
  IOP_TRACE();
  switch (code) {
  case 200:
    return IOP_STR("OK").toString();
  case 500:
    return IOP_STR("SERVER_ERROR").toString();
  case 401:
    return IOP_STR("UNAUTHORIZED").toString();
  }
  return std::to_string(code);
}
}

namespace iop_hal {
auto networkStatus(const int code) noexcept -> std::optional<iop::NetworkStatus> {
  IOP_TRACE();
  switch (code) {
  case 200:
    return iop::NetworkStatus::OK;
  case 500:
    return iop::NetworkStatus::BROKEN_SERVER;
  case 401:
    return iop::NetworkStatus::UNAUTHORIZED;
  }
  return std::nullopt;
}
void HTTPClient::headersToCollect(std::vector<std::string> headers) noexcept { (void) headers; }
auto Response::header(iop::StaticString key) const noexcept -> std::optional<std::string> { (void) key; return std::nullopt; }
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept { (void) key; (void) value; }
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(std::string_view key, std::string_view value) noexcept { (void) key; (void) value; }
void Session::setAuthorization(std::string auth) noexcept { (void) auth; }
auto Session::sendRequest(const std::string method, const std::string_view data) noexcept -> Response { (void) method; (void) data; return Response(500); }

auto HTTPClient::setup() noexcept -> void {}
HTTPClient::HTTPClient() noexcept {}
HTTPClient::~HTTPClient() noexcept {}

auto HTTPClient::begin(std::string_view uri, std::function<Response(Session &)> func) noexcept -> Response { (void) uri; (void) func; return Response(iop::NetworkStatus::OK); }
}